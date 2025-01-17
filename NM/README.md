
# Request - response format

## NS - SS

1. REQUEST_REGISTER
S: REQUEST_REGISTER ip|nm_port|client_port , 0, OK
N: ACK  , id, OK/REGISTER_FAILED

2. ADD_PATH
S: ADD_PATH paths|..... , id, OK
N: ACK, id, OK/..

3. DELETE_PATH
S: DELETE_PATH paths|..... , id, OK
N: ACK, id, OK/..

4. READ_START
S: READING path, id, OK
N: ACK, id, OK

5. WRITE_START
S: WRITING path, id, ok
N: ACK, id, OK

6. READ_DONE
S: READ_DONE path, id, ok
N: ACK, id, OK

7. WRITE_DONE
S: WRITE_DONE path, id, ok
N: ACK, id, OK

4. CREATE_FILE/DIR
N: CREATE_FILE/DIR 