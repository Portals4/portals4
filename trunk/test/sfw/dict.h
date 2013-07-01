/*
 * dict.h
 */

#ifndef DICT_H
#define DICT_H

enum {
	TYPE_NODE,
	TYPE_ATTR,
	TYPE_OPT,
};

enum {
	NODE_SET,
	NODE_CHECK,
	NODE_IF,
	NODE_IFDEF,
	NODE_ELSE,
	NODE_ARG_CHECK,
	NODE_TEST,
	NODE_THREADS,
	NODE_SUBTEST,
	NODE_DESC,
	NODE_COMMENT,
	NODE_REPEAT,
	NODE_MSLEEP,
	NODE_TIME,
	NODE_BARRIER,
	NODE_OMPI_RT,
	NODE_PTL,
	NODE_PTL_INIT,
	NODE_PTL_FINI,
	NODE_PTL_NI,
	NODE_PTL_NI_INIT,
	NODE_PTL_NI_FINI,
	NODE_PTL_NI_STATUS,
	NODE_PTL_NI_HANDLE,
	NODE_PTL_PT,
	NODE_PTL_PT_ALLOC,
	NODE_PTL_PT_FREE,
	NODE_PTL_PT_DISABLE,
	NODE_PTL_PT_ENABLE,
	NODE_PTL_GET_UID,
	NODE_PTL_GET_ID,
	NODE_PTL_MD,
	NODE_PTL_MD_BIND,
	NODE_PTL_MD_RELEASE,
	NODE_PTL_LE,
	NODE_PTL_LE_APPEND,
	NODE_PTL_LE_UNLINK,
	NODE_PTL_LE_SEARCH,
	NODE_PTL_ME,
	NODE_PTL_ME_APPEND,
	NODE_PTL_ME_UNLINK,
	NODE_PTL_ME_SEARCH,
	NODE_PTL_EQ,
	NODE_PTL_EQ_ALLOC,
	NODE_PTL_EQ_FREE,
	NODE_PTL_EQ_GET,
	NODE_PTL_EQ_WAIT,
	NODE_PTL_EQ_POLL,
	NODE_PTL_CT,
	NODE_PTL_CT_ALLOC,
	NODE_PTL_CT_FREE,
	NODE_PTL_CT_GET,
	NODE_PTL_CT_WAIT,
	NODE_PTL_CT_POLL,
	NODE_PTL_CT_SET,
	NODE_PTL_CT_INC,
	NODE_PTL_CT_CANCEL_TRIG,
	NODE_PTL_PUT,
	NODE_PTL_GET,
	NODE_PTL_ATOMIC,
	NODE_PTL_ATOMIC_SYNC,
	NODE_PTL_FETCH,
	NODE_PTL_SWAP,
	NODE_PTL_TRIG_PUT,
	NODE_PTL_TRIG_GET,
	NODE_PTL_TRIG_ATOMIC,
	NODE_PTL_TRIG_FETCH,
	NODE_PTL_TRIG_SWAP,
	NODE_PTL_TRIG_CT_INC,
	NODE_PTL_TRIG_CT_SET,
	NODE_PTL_START_BUNDLE,
	NODE_PTL_END_BUNDLE,
	NODE_PTL_HANDLE_IS_EQUAL,
	NODE_PTL_SET_MAP,
	NODE_PTL_GET_MAP,

	/* general */
	ATTR_RET,
	ATTR_ERR,
	ATTR_PTR,
	ATTR_COUNT,
	ATTR_THREAD_ID,
	ATTR_NID,
	ATTR_PID,
	ATTR_RANK,
	ATTR_UID,
	ATTR_LIST,
	ATTR_USER_PTR,

	/* ni */
	ATTR_IFACE,
	ATTR_NI_OPT,
	ATTR_DESIRED_MAX_ENTRIES,
	ATTR_DESIRED_MAX_OVER,
	ATTR_DESIRED_MAX_MDS,
	ATTR_DESIRED_MAX_EQS,
	ATTR_DESIRED_MAX_CTS,
	ATTR_DESIRED_MAX_PT_INDEX,
	ATTR_DESIRED_MAX_IOVECS,
	ATTR_DESIRED_MAX_LIST_SIZE,
	ATTR_DESIRED_MAX_MSG_SIZE,
	ATTR_DESIRED_MAX_ATOMIC_SIZE,
	ATTR_DESIRED_MAX_VOLATILE_SIZE,
	ATTR_DESIRED_MAX_ORDERED_SIZE,
	ATTR_DESIRED_MAX_UNEXPECTED_HEADERS,
	ATTR_DESIRED_FEATURES,
	ATTR_ACTUAL_MAX_ENTRIES,
	ATTR_ACTUAL_MAX_OVER,
	ATTR_ACTUAL_MAX_MDS,
	ATTR_ACTUAL_MAX_EQS,
	ATTR_ACTUAL_MAX_CTS,
	ATTR_ACTUAL_MAX_PT_INDEX,
	ATTR_ACTUAL_MAX_IOVECS,
	ATTR_ACTUAL_MAX_LIST_SIZE,
	ATTR_ACTUAL_MAX_MSG_SIZE,
	ATTR_ACTUAL_MAX_ATOMIC_SIZE,
	ATTR_ACTUAL_MAX_VOLATILE_SIZE,
	ATTR_ACTUAL_MAX_ORDERED_SIZE,
	ATTR_ACTUAL_MAX_UNEXPECTED_HEADERS,
	ATTR_ACTUAL_FEATURES,
	ATTR_MAP_SIZE,
	ATTR_DESIRED_MAP_PTR,
	ATTR_ACTUAL_MAP_PTR,
	ATTR_NI_HANDLE,
	ATTR_SR_INDEX,
	ATTR_SR_VALUE,
	ATTR_HANDLE,

	/* pt */
	ATTR_PT_OPT,
	ATTR_PT_INDEX_REQ,
	ATTR_PT_INDEX,

	/* md */
	ATTR_IOV_BASE,
	ATTR_IOVEC_LEN,

	ATTR_MD_START,
	ATTR_MD_LENGTH,
	ATTR_MD_OPT,
	ATTR_MD_HANDLE,
	ATTR_MD_DATA,

	/* le */
	ATTR_LE_START,
	ATTR_LE_LENGTH,
	ATTR_LE_OPT,
	ATTR_LE_DATA,
	ATTR_LE_HANDLE,
	ATTR_SEARCH_OP,

	/* me */
	ATTR_ME_START,
	ATTR_ME_LENGTH,
	ATTR_ME_MIN_FREE,
	ATTR_ME_OPT,
	ATTR_ME_MATCH,
	ATTR_ME_IGNORE,
	ATTR_ME_DATA,
	ATTR_ME_HANDLE,

	/* eq */
	ATTR_TIME,
	ATTR_WHICH,
	ATTR_WHICH_PTR,
	ATTR_EQ_COUNT,
	ATTR_EQ_HANDLE,

	ATTR_EVENT_TYPE,
	ATTR_EVENT_NID,
	ATTR_EVENT_PID,
	ATTR_EVENT_RANK,
	ATTR_EVENT_PT_INDEX,
	ATTR_EVENT_UID,
	ATTR_EVENT_MATCH,
	ATTR_EVENT_RLENGTH,
	ATTR_EVENT_MLENGTH,
	ATTR_EVENT_OFFSET,
	ATTR_EVENT_START,
	ATTR_EVENT_USER_PTR,
	ATTR_EVENT_HDR_DATA,
	ATTR_EVENT_NI_FAIL,
	ATTR_EVENT_ATOM_OP,
	ATTR_EVENT_ATOM_TYPE,
	ATTR_EVENT_PTL_LIST,

	/* CT */
	ATTR_CT_HANDLE,
	ATTR_CT_EVENT_SUCCESS,
	ATTR_CT_EVENT_FAILURE,
	ATTR_CT_TEST,

	ATTR_LENGTH,
	ATTR_OFFSET,
	ATTR_OPERAND,
	ATTR_TYPE,
	ATTR_LOC_OFFSET,
	ATTR_LOC_GET_OFFSET,
	ATTR_LOC_PUT_OFFSET,
	ATTR_REM_OFFSET,
	ATTR_ATOM_OP,
	ATTR_ATOM_TYPE,
	ATTR_MATCH,
	ATTR_GET_MD_HANDLE,
	ATTR_PUT_MD_HANDLE,
	ATTR_ACK_REQ,
	ATTR_HANDLE1,
	ATTR_HANDLE2,
	ATTR_THRESHOLD,

	/* put */
	ATTR_TARGET_ID,

	/* maps */
	ATTR_GET_MAP_SIZE,

	/* options */
	OPT_CHECK_POINTER,
};

struct dict_entry {
	struct dict_entry	*next;
	char			*name;
	int			type;
	int			token;
};

struct dict_entry *lookup(const char *name);

struct dict_entry *insert(const char *name);

void init_dict(void);

#endif /* DICT_H */
