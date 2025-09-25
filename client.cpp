/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Robbie Hahn
	UIN: 234001615
	Date: 9/16/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> // wait

using namespace std;


int main (int argc, char *argv[]) {
    int opt;
    int p = -1;
    double t = -1.0;
    int e = -1;
    int buffercapacity = MAX_MESSAGE;
    string filename = "";
    bool use_new_channel = false;
while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
    switch (opt) {
        case 'p': p = atoi(optarg); break;
        case 't': t = atof(optarg); break;
        case 'e': e = atoi(optarg); break;
        case 'f': filename = optarg; break;
        case 'm': buffercapacity = atoi(optarg); break;
        case 'c': use_new_channel = true; break;
    }
}

	// start the server by forking and starting a new process
	// you need to add code to this function
	// to actually start the server process
	pid_t s = fork();
	if (s == 0) {
    // child process: start server
		char* args[] = {(char*)"./server", nullptr};
		execvp(args[0], args);
		cerr << "Exec failed for server\n";
		return 1;
}
sleep(1); // let the server start up

	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
    FIFORequestChannel* active_chan = &chan;
    if (use_new_channel) {
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        active_chan->cwrite(&m, sizeof(MESSAGE_TYPE));
        char new_channel_name[100];
        active_chan->cread(new_channel_name, 100);
        FIFORequestChannel* new_chan = new FIFORequestChannel(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
        active_chan = new_chan;
        cout << "New channel created: " << new_channel_name << endl;
    }
    if (p != -1 && t != -1.0 && e != -1) {
        datamsg dmsg(p, t, e);
        active_chan->cwrite(&dmsg, sizeof(datamsg));
        double ecg_value;
        active_chan->cread(&ecg_value, sizeof(double));
        cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << ecg_value << endl;
    }
    if (p != -1 && t == 0.0 && filename == "") {
        mkdir("received", 0777);  // ensure directory exists
        ofstream out("received/x1.csv");
        for (int i = 0; i < 1000; ++i) {
            double time = i * 0.004;
            datamsg msg1(p, time, 1);
            datamsg msg2(p, time, 2);
            double ecg1, ecg2;

            // chan.cwrite(&msg1, sizeof(datamsg));
            // chan.cread(&ecg1, sizeof(double));
            // chan.cwrite(&msg2, sizeof(datamsg));
            // chan.cread(&ecg2, sizeof(double));
            active_chan->cwrite(&msg1, sizeof(datamsg));
            active_chan->cread(&ecg1, sizeof(double));
            active_chan->cwrite(&msg2, sizeof(datamsg));
            active_chan->cread(&ecg2, sizeof(double));


            out << time << "," << ecg1 << "," << ecg2 << endl;
        }
        out.close();
    }


if (filename != "") {
    mkdir("received", 0777);  // ensure directory exists

    // Step 1: Get file size
    filemsg fmsg(0, 0);
    int request_len = sizeof(filemsg) + filename.size() + 1;
    char* request = new char[request_len];
    memcpy(request, &fmsg, sizeof(filemsg));
    strcpy(request + sizeof(filemsg), filename.c_str());
    active_chan->cwrite(request, request_len);
    __int64_t file_size;
    active_chan->cread(&file_size, sizeof(__int64_t));
    delete[] request;
    // Step 2: Prepare output file
    string outpath = "received/" + filename;
    ofstream outfile(outpath, ios::binary);
    if (!outfile.is_open()) {
        cerr << "Failed to open output file: " << outpath << endl;
        exit(1);
    }

    // Step 3: Request file in chunks
    __int64_t offset = 0;
    while (offset < file_size) {
        int chunk_size = min(buffercapacity, static_cast<int>(file_size - offset));
        filemsg chunk_msg(offset, chunk_size);
        int msg_len = sizeof(filemsg) + filename.size() + 1;
        char* msg = new char[msg_len];
        memcpy(msg, &chunk_msg, sizeof(filemsg));
        strcpy(msg + sizeof(filemsg), filename.c_str());

        active_chan->cwrite(msg, msg_len);
        char* response = new char[chunk_size];
        active_chan->cread(response, chunk_size);
        outfile.write(response, chunk_size);
        offset += chunk_size;
        delete[] msg;
        delete[] response;
    }

    outfile.close();
}




	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    if (use_new_channel && active_chan != &chan) {
    active_chan->cwrite(&m, sizeof(MESSAGE_TYPE));
    delete active_chan;
}
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	waitpid(s, nullptr, 0); // wait for the server to finish
	return 0;
}
