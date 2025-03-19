#!/usr/bin/env bats

# Setup function to start the server in the background
setup() {
    ./dsh -s -i 127.0.0.1 -p 7890 &
    SERVER_PID=$!
    sleep 1  # Give the server time to start
}

# Teardown function to stop the server after tests
teardown() {
    kill $SERVER_PID
    sleep 1
}

# Test if server starts successfully
@test "Server starts and listens on port 7890" {
    run nc -zv 127.0.0.1 7890
    [ "$status" -eq 0 ]
}

# Test if client can connect to server
@test "Client can connect to the server" {
    run ./dsh -c -i 127.0.0.1 -p 7890 <<< "exit"
    [ "$status" -eq 0 ]
}

# Test executing a simple command
@test "Execute 'ls' command remotely" {
    run bash -c "echo 'ls' | ./dsh -c -i 127.0.0.1 -p 7890"
    [ "$status" -eq 0 ]
    [[ "$output" =~ "dsh_cli.c" ]]  
}



# Test 'cd' command
@test "Change directory using 'cd' command" {
    run bash -c "echo 'cd /tmp && pwd'"
    [ "$status" -eq 0 ]
    [[ "$output" =~ "/tmp" ]]
}

# Test 'exit' command
@test "Exit command terminates client session" {
    run bash -c "echo 'exit' | ./dsh -c -i 127.0.0.1 -p 7890"
    [ "$status" -eq 0 ]
}

# Test 'stop-server' command
@test "Stop-server command terminates the server" {
    run bash -c "echo 'stop-server' | ./dsh -c -i 127.0.0.1 -p 7890"
    [ "$status" -eq 0 ]
    sleep 1
    run nc -zv 127.0.0.1 7890
    [ "$status" -ne 0 ]
}
