#ifndef PTL_INTERNAL_EQ_H
#define PTL_INTERNAL_EQ_H

int PtlInternalEQHandleValidator(
    ptl_handle_eq_t handle,
    int none_ok);
void PtlInternalEQPush(
    ptl_handle_eq_t handle,
    ptl_event_t * event);

#endif
