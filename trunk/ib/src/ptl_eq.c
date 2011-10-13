/*
 * ptl.c - Portals API
 */

#include "ptl_loc.h"

void eq_cleanup(void *arg)
{
	eq_t *eq = arg;
	ni_t *ni = obj_to_ni(eq);

	pthread_spin_lock(&ni->obj.obj_lock);
	ni->current.max_eqs--;
	pthread_spin_unlock(&ni->obj.obj_lock);

	if (eq->eqe_list)
		free(eq->eqe_list);
	eq->eqe_list = NULL;
}

/* can return
PTL_OK
PTL_NO_INIT
PTL_ARG_INVALID
PTL_NO_SPACE
PTL_ARG_INVALID
*/
int PtlEQAlloc(ptl_handle_ni_t ni_handle,
	       ptl_size_t count,
	       ptl_handle_eq_t *eq_handle)
{
	int err;
	ni_t *ni;
	eq_t *eq;

	err = get_gbl();
	if (unlikely(err))
		return err;

	err = to_ni(ni_handle, &ni);
	if (unlikely(err))
		goto err1;

	if (!ni) {
		err = PTL_ARG_INVALID;
		goto err1;
	}

	err = eq_alloc(ni, &eq);
	if (unlikely(err))
		goto err2;

	eq->eqe_list = calloc(count, sizeof(*eq->eqe_list));
	if (!eq->eqe_list) {
		err = PTL_NO_SPACE;
		goto err3;
	}

	eq->count = count;
	eq->producer = 0;
	eq->consumer = 0;
	eq->prod_gen = 0;
	eq->cons_gen = 0;
	eq->overflow = 0;
	eq->interrupt = 0;

	pthread_spin_lock(&ni->obj.obj_lock);
	ni->current.max_eqs++;
	/* eq_release will decrement count and free eqe_list */
	if (unlikely(ni->current.max_eqs > ni->limits.max_eqs)) {
		pthread_spin_unlock(&ni->obj.obj_lock);
		err = PTL_NO_SPACE;
		goto err3;
	}
	pthread_spin_unlock(&ni->obj.obj_lock);

	*eq_handle = eq_to_handle(eq);

	ni_put(ni);
	gbl_put();
	return PTL_OK;

err3:
	eq_put(eq);
err2:
	ni_put(ni);
err1:
	gbl_put();
	return err;
}

/* can return
PTL_OK
PTL_NO_INIT
PTL_ARG_INVALID
*/
int PtlEQFree(ptl_handle_eq_t eq_handle)
{
	int err;
	eq_t *eq;
	ni_t *ni;

	err = get_gbl();
	if (unlikely(err))
		return err;

	err = to_eq(eq_handle, &eq);
	if (unlikely(err))
		goto err1;

	if (!eq) {
		err = PTL_ARG_INVALID;
		goto err1;
	}

	ni = obj_to_ni(eq);
	if (!ni) {
		err = PTL_ARG_INVALID;
		goto err1;
	}

	if (atomic_read(&ni->eq_waiting)) {
		eq->interrupt = 1;
		pthread_mutex_lock(&ni->eq_wait_mutex);
		pthread_cond_broadcast(&ni->eq_wait_cond);
		pthread_mutex_unlock(&ni->eq_wait_mutex);
	}

	eq_put(eq);
	eq_put(eq);
	gbl_put();
	return PTL_OK;

err1:
	gbl_put();
	return err;
}

static int get_event(eq_t *eq, ptl_event_t *event)
{
	int ret;

	pthread_spin_lock(&eq->obj.obj_lock);
	if ((eq->producer == eq->consumer) &&
	    (eq->prod_gen == eq->cons_gen)) {
		pthread_spin_unlock(&eq->obj.obj_lock);
		return PTL_EQ_EMPTY;
	}

	if (eq->overflow) {
		ret = PTL_EQ_DROPPED;
		eq->overflow = 0;
	} else
		ret = PTL_OK;

	*event = eq->eqe_list[eq->consumer++].event;
	if (eq->consumer >= eq->count) {
		eq->consumer = 0;
		eq->cons_gen++;
	}
	pthread_spin_unlock(&eq->obj.obj_lock);
	return ret;
}

int PtlEQGet(ptl_handle_eq_t eq_handle,
	     ptl_event_t *event)
{
	int err;
	eq_t *eq;

	err = get_gbl();
	if (unlikely(err))
		return err;

	err = to_eq(eq_handle, &eq);
	if (unlikely(err))
		goto err1;

	if (!eq) {
		err = PTL_ARG_INVALID;
		goto err1;
	}

	err = get_event(eq, event);
	eq_put(eq);
	gbl_put();
	return err;

err1:
	gbl_put();
	return err;
}

int PtlEQWait(ptl_handle_eq_t eq_handle,
	      ptl_event_t *event)
{
	int ret;
	eq_t *eq;
	ni_t *ni;
	int nloops;

	ret = get_gbl();
	if (unlikely(ret))
		return ret;

	ret = to_eq(eq_handle, &eq);
	if (unlikely(ret))
		goto err1;

	if (!eq) {
		ret = PTL_ARG_INVALID;
		goto err1;
	}

	ni = obj_to_ni(eq);
	if (!ni) {
		ret = PTL_ARG_INVALID;
		goto done;
	}

	/* First try with just spinning */
	nloops = 100000;
	do {
		if (eq->interrupt) {
			ret = PTL_INTERRUPTED;
			goto done;
		}

		if (nloops) {
			/* Spin again. */
			nloops --;
			ret = get_event(eq, event);
		} else {
			/* Done spinning. Conditionally block until interrupted or
			 * event present */

			/* Serialize for blocking on empty */
			pthread_mutex_lock(&ni->eq_wait_mutex);

			ret = get_event(eq, event);
			if (ret == PTL_EQ_EMPTY) {
				atomic_inc(&ni->eq_waiting);
				pthread_cond_wait(&ni->eq_wait_cond, &ni->eq_wait_mutex);
				atomic_dec(&ni->eq_waiting);
			}

			pthread_mutex_unlock(&ni->eq_wait_mutex);
		}
	} while (ret == PTL_EQ_EMPTY);

 done:
	eq_put(eq);
	gbl_put();
	return ret;

err1:
	gbl_put();
	return ret;
}

int PtlEQPoll(const ptl_handle_eq_t *eq_handles,
	      unsigned int size,
	      ptl_time_t timeout,
	      ptl_event_t *event,
	      unsigned int *which)
{
	int err;
	ni_t *ni = NULL;
	eq_t **eq = NULL;
	struct timespec expire;
	int i;
	int last_eq;

	err = get_gbl();
	if (unlikely(err))
		return err;

	if (size == 0) {
		WARN();
		err = PTL_ARG_INVALID;
		goto done;
	}

	eq = malloc(size*sizeof(*eq));
	if (!eq) {
		WARN();
		err = PTL_NO_SPACE;
		goto done;
	}

	for (i = 0; i < size; i++) {
		err = to_eq(eq_handles[i], &eq[i]);
		if (unlikely(err || !eq[i])) {
			last_eq = i;
			WARN();
			err = PTL_ARG_INVALID;
			goto done2;
		}
	}

	last_eq = size;

	ni = obj_to_ni(eq[0]);

	for (i = 1; i < size; i++) {
		if (obj_to_ni(eq[i]) != ni) {
			WARN();
			err = PTL_ARG_INVALID;
			goto done2;
		}
	}

	if (timeout != PTL_TIME_FOREVER) {
		clock_gettime(CLOCK_REALTIME, &expire);
		expire.tv_nsec += 1000000UL * timeout;
		expire.tv_sec += expire.tv_nsec/1000000000UL;
		expire.tv_nsec = expire.tv_nsec % 1000000000UL;
	}

	pthread_mutex_lock(&ni->eq_wait_mutex);
	err = 0;
	while (!err) {
		for (i = 0; i < size; i++) {
			if (eq[i]->interrupt) {
				pthread_mutex_unlock(&ni->eq_wait_mutex);
				err = PTL_INTERRUPTED;
				goto done2;
			}

			err = get_event(eq[i], event);
			if (err != PTL_EQ_EMPTY) {
				*which = i;
				pthread_mutex_unlock(&ni->eq_wait_mutex);
				goto done2;
			}
		}

		atomic_inc(&ni->eq_waiting);
		if (timeout == PTL_TIME_FOREVER)
			pthread_cond_wait(&ni->eq_wait_cond,
				&ni->eq_wait_mutex);
		else
			err = pthread_cond_timedwait(&ni->eq_wait_cond,
				&ni->eq_wait_mutex, &expire);
		atomic_dec(&ni->eq_waiting);
	}
	pthread_mutex_unlock(&ni->eq_wait_mutex);
	err = PTL_EQ_EMPTY;

done2:
	for (i = 0; i < last_eq; i++)
		eq_put(eq[i]);

done:
	if (eq)
		free(eq);

	gbl_put();
	return err;
}

void event_dump(ptl_event_t *ev)
{
	printf("ev:		%p\n", ev);
	printf("ev->type:	%d\n", ev->type);
	printf("ev->ni_fail_type:   %d\n", ev->ni_fail_type);
	printf("\n");
}

void make_init_event(xi_t *xi, eq_t *eq, ptl_event_kind_t type, void *start)
{
	ptl_event_t *ev;
	ni_t *ni;

	pthread_spin_lock(&eq->obj.obj_lock);
	if ((eq->prod_gen != eq->cons_gen) && (eq->producer >= eq->consumer)) {
		WARN();
		eq->overflow = 1;
	}

	eq->eqe_list[eq->producer].generation = eq->prod_gen;
	ev = &eq->eqe_list[eq->producer].event;

	ev->type		= type;
	ev->initiator		= xi->target;
	ev->pt_index		= xi->pt_index;
	ev->uid			= xi->uid;
	ev->match_bits		= xi->match_bits;
	ev->rlength		= xi->rlength;
	ev->mlength		= xi->mlength;
	ev->remote_offset	= xi->moffset;
	ev->start		= start;
	ev->user_ptr		= xi->user_ptr;
	ev->hdr_data		= xi->hdr_data;
	ev->ni_fail_type	= xi->ni_fail;
	ev->atomic_operation	= xi->atom_op;
	ev->atomic_type		= xi->atom_type;

	eq->producer++;
	if (eq->producer >= eq->count) {
		eq->producer = 0;
		eq->prod_gen++;
	}
	pthread_spin_unlock(&eq->obj.obj_lock);

	/* Handle case where waiters have blocked */
	ni = obj_to_ni(eq);
	pthread_mutex_lock(&ni->eq_wait_mutex);
	if (atomic_read(&ni->eq_waiting))
		pthread_cond_broadcast(&ni->eq_wait_cond);
	pthread_mutex_unlock(&ni->eq_wait_mutex);

	if (debug) event_dump(ev);
}

void make_target_event(xt_t *xt, eq_t *eq, ptl_event_kind_t type,
		       void *user_ptr, void *start)
{
	ptl_event_t *ev;
	ni_t *ni;

	pthread_spin_lock(&eq->obj.obj_lock);
	if ((eq->prod_gen != eq->cons_gen) && (eq->producer >= eq->consumer)) {
		WARN();
		eq->overflow = 1;
	}

	eq->eqe_list[eq->producer].generation = eq->prod_gen;
	ev = &eq->eqe_list[eq->producer].event;

	ev->type		= type;
	ev->initiator		= xt->initiator;
	ev->pt_index		= xt->pt_index;
	ev->uid			= xt->uid;
	ev->match_bits		= xt->match_bits;
	ev->rlength		= xt->rlength;
	ev->mlength		= xt->mlength;
	ev->remote_offset	= xt->roffset;
	ev->start		= start;
	ev->user_ptr		= user_ptr;
	ev->hdr_data		= xt->hdr_data;
	ev->ni_fail_type	= xt->ni_fail;
	ev->atomic_operation	= xt->atom_op;
	ev->atomic_type		= xt->atom_type;

	eq->producer++;
	if (eq->producer >= eq->count) {
		eq->producer = 0;
		eq->prod_gen++;
	}
	pthread_spin_unlock(&eq->obj.obj_lock);

	/* Handle case where waiters have blocked */
	ni = obj_to_ni(eq);
	pthread_mutex_lock(&ni->eq_wait_mutex);
	if (atomic_read(&ni->eq_waiting))
		pthread_cond_broadcast(&ni->eq_wait_cond);
	pthread_mutex_unlock(&ni->eq_wait_mutex);

	if (debug) event_dump(ev);
}

/* Makes an LE/ME event */
void make_le_event(le_t *le, eq_t *eq, ptl_event_kind_t type,
		   ptl_ni_fail_t fail_type)
{
	ptl_event_t *ev;
	ni_t *ni;

	pthread_spin_lock(&eq->obj.obj_lock);
	if ((eq->prod_gen != eq->cons_gen) && (eq->producer >= eq->consumer)) {
		WARN();
		eq->overflow = 1;
	}

	eq->eqe_list[eq->producer].generation = eq->prod_gen;
	ev = &eq->eqe_list[eq->producer].event;

	ev->type = type;
	ev->pt_index = le->pt_index;
	ev->user_ptr = le->user_ptr;
	ev->ni_fail_type = fail_type;

	eq->producer++;
	if (eq->producer >= eq->count) {
		eq->producer = 0;
		eq->prod_gen++;
	}
	pthread_spin_unlock(&eq->obj.obj_lock);

	/* Handle case where waiters have blocked */
	ni = obj_to_ni(eq);
	pthread_mutex_lock(&ni->eq_wait_mutex);
	if (atomic_read(&ni->eq_waiting))
		pthread_cond_broadcast(&ni->eq_wait_cond);
	pthread_mutex_unlock(&ni->eq_wait_mutex);

	if (debug) event_dump(ev);
}