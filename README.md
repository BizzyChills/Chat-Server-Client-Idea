
# handin instructions

1. Add files to this directory
2. Add files in this directory to `manifest` files. eg: `find > manifest`, then
remove any files you don't want to submit.
3. Document how to build/run your server in this file.
4. Document how to build/run your client in this file.
5. Document the features you would like me to assess in this file. I encourage
you to verify them with me ahead of time to see if they qualify. Identify
and briefly describe how to use each feature.


# Features

1. Use "> general" or "> niche" to change channels respecively
2. Use ">@ <username>" to enter a user's DM. If username is own username, lists all private message requests. If username is absent, lists all memebers in the user's channel (or the DM their in)
3. Use "%<command>" to escape a command at the start of a message. Try escaping the escape.
4. (Make sure you use the HELLO command noted below before trying these. ">?" for help)

# How to run server

1. Use makefile in root of project
                or
2. Use makefile in ./client (make -C ./server)
                then
3. Run "./server/chat-server <port>" to start the server

# How to run client

1. Use makefile in root of project
                or
2. Use makefile in ./client (make -C ./client)
                then
3. Run "./client/chat-client <port>" to start the client 
4. Send "HELLO <uname>" to join the server. Send ">?" for a helpful list of commands
