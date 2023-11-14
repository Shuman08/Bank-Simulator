Command prompt : 
1. In command prompt , cd to the directory stored this file.
2. type the command : gcc -main.c -o main -pthread
3. run the program with : main m tc tw td ti


VS Code terminal:
1. Open Visual Studio Code
2. Open new folder and select the folder you stored the extracted files
3. Open Visual Studio Code terminal
4. Compile the program with the command : gg -main.c -o main -pthread
5. Run the program with ./main m tc tw td ti

Where m is the size of queue, tc is the time in second for customer to arrive queue, 
tw is the duration in second  to serve withdrawal
td is the duration in second  to serve deposit
ti is the duration in second  to serve asking information.

*Must add -pthread behind the compiling command.
*When wrong arguments is entered during running the program, an usage message will be printed in console. Just need to rerun it with correct arguments.