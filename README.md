# WinServer
Example of native WinSock2 API Server with parallel clients servicing.

# Usage
_WinServer.exe [--threads-num=<number_of_work_threads>] [--port=<listen_port>]_

**Default port:** 27015

**Default threads num:** ```(system_concurrency) * 2 - 1```

# API
Server is processing **POST** requests with ```application/json``` content type. Data payload must contain ```data``` root field. Response contains ```data``` field with **reversed request-data** (code 200) or ```error``` field with **error description** (code 400).

**Example:**

  ```{"data": "123456789"} ---> {"data": "987654321"}```

## Request example
  ```curl http://localhost:27015 -X POST -H "Content-Type: application/json" -d "{\"data\":\"123456789\"}"```
  
## Response example
```
HTTP /1.1 200 OK
Content-Type: application/json
Content-Length: 33
Server: WinServer

"{\"data\":\"987654321\"}"
```

# Components
* CApplication - main app class which encapsulates common server's logic, network processing and work thread pool.
* CHttpParser - requests validator and response builder
* CLogger - encapsulates logging logic (parallel logging into stdout, stderr and ```main.log```)
* SInputParams - struct for launch params handling
