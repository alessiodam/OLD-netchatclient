# TCP Server Documentation

Welcome to the documentation for the TCP server. This server provides a range of functionalities for client developers to interact with. Below, you'll find detailed explanations for each supported command.

## Command List

### USERNAME
- **Description**: Used to set the username for authentication.
- **Input Format**: `USERNAME:your_username`
- **Example**: `USERNAME:john_doe`

### TOKEN
- **Description**: Used to provide authentication token for secure access.
- **Input Format**: `TOKEN:your_token`
- **Example**: `TOKEN:abc123xyz`

### RTC_CHAT
- **Description**: Send a real-time chat message to all connected clients.
- **Input Format**: `RTC_CHAT:message_content`
- **Example**: `RTC_CHAT:Hello, everyone!`

### ACCOUNT_INFO
- **Description**: Retrieve account information for the authenticated user.
- **Input Format**: `ACCOUNT_INFO`
- **Output**: A semicolon-separated string containing user details.

### GET_BUCKET_CONTENT
- **Description**: Retrieve the contents of a specified bucket.
- **Input Format**: `GET_BUCKET_CONTENT:bucket_name`
- **Output**: A semicolon-separated list of filenames in the bucket.

### GET_BUCKET_FILE
- **Description**: Retrieve a specific file from a bucket.
- **Input Format**: `GET_BUCKET_FILE:bucket_name:filename`
- **Output**: The content of the requested file.

### DELETE_BUCKET_FILE
- **Description**: Delete a file from a bucket.
- **Input Format**: `DELETE_BUCKET_FILE:bucket_name:filename`
- **Output**: Confirmation or error message.

### CREATE_BUCKET
- **Description**: Create a new bucket.
- **Input Format**: `CREATE_BUCKET:bucket_name`
- **Output**: Confirmation or error message.

### DELETE_BUCKET
- **Description**: Delete an existing bucket.
- **Input Format**: `DELETE_BUCKET:bucket_name`
- **Output**: Confirmation or error message.

### UPLOAD_TO_BUCKET
- **Description**: Upload a file to a bucket (In Development).
- **Input Format**: `UPLOAD_TO_BUCKET:bucket_name:file_name`
- **Additional Steps**: Send the file content after the command.

## Usage Example

To send a real-time chat message, use the following format:
