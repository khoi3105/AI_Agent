# AI_Agent
oop project 

# build command
stay at the parent dir
g++-16 src/client/ollama_client.cpp src/main.cpp -o build/main -lcurl && export $(cat .env | xargs)  && ./build/main