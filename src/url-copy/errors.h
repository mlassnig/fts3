#pragma once

/*ERROR_SCOPE*/
#define TRANSFER "TRANSFER"
#define DESTINATION "DESTINATION"
#define AGENT "GENERAL_FAILURE"
#define SOURCE "SOURCE"

/*ERROR_PHASE*/
#define TRANSFER_PREPARATION "TRANSFER_PREPARATION"
#define TRANSFER "TRANSFER"
#define TRANSFER_FINALIZATION "TRANSFER_FINALIZATION"
#define ALLOCATION "ALLOCATION"
#define TRANSFER_SERVICE "TRANSFER_SERVICE"

/*REASON_CLASS*/
#define USER_ERROR "USER_ERROR"
#define INTERNAL_ERROR "INTERNAL_ERROR"
#define CONNECTION_ERROR "CONNECTION_ERROR"
#define REQUEST_TIMEOUT "REQUEST_TIMEOUT"
#define LOCALITY "LOCALITY"
#define ABORTED "ABORTED"
#define GRIDFTP_ERROR "GRIDFTP_ERROR"
#define HTTP_TIMEOUT "HTTP_TIMEOUT"
#define INVALID_PATH "INVALID_PATH"
#define STORAGE_INTERNAL_ERROR ""
#define GENERAL_FAILURE "GENERAL_FAILURE"
#define SECURITY_ERROR "SECURITY_ERROR"
