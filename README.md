# AI_Agent
oop project 

# build command
stay at the parent dir
g++-16 -std=c++26 src/client/ollama_client.cpp src/main.cpp src/utils/*.cpp -o build/main -lcurl && export $(cat .env | xargs)  && ./build/main