/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: 	Rodrigo Orozco
	UIN:	933000862
	Date:	9/29
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sstream>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	// -1 for data request flag
	int p = -1;
	double t = -1.0;
	int e = -1;
	size_t buff_cap = MAX_MESSAGE;      // optinal buffer capacity
	string filename = "";

	

	//Add other arguments here
	while ((opt = getopt(argc, argv, "p:t:e:f:m:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				buff_cap = atoi (optarg);
				break;
		}
	}

	//Task 1:
	//Run the server process as a child of the client process
	
	pid_t child = fork();
	if (child == 0)
	{
		char* cmd[] = {(char*) "./server", (char*)"-m", (char*)to_string(buff_cap).c_str(), nullptr};	// arguments for server
		execvp("./server", cmd);																		// run server
	}

	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	//Task 4:
	//Request a new channel
	
	//Task 2:
	//Request data points

	if (p != -1 && t != -1.0 && e != -1)
	{	// single data point
		char buf[MAX_MESSAGE];
		// datamsg x(1, 0.0, 1);
		datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	else if (p != -1)
	{
		ofstream received("received/x1.csv");
		string path = "BIMDC/9.csv";
		ifstream bimdc(path);
		string line;
		int count = 1;

		while (getline(bimdc, line))
		{
			if (count > 1000){
				break;
			}

			string item;
			vector<string> parsed_line;
			std::stringstream ss(line);

			while (getline(ss, item, ','))
			{
				parsed_line.push_back(item);
			}
			t = stof(parsed_line[0]);
			e = 1;

	
			char buf[MAX_MESSAGE];
			datamsg x(p, t, e);
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double reply;
			chan.cread(&reply, sizeof(double));
			received << t << ", " << reply;
			received.flush();

			e = 2;
			char buf2[MAX_MESSAGE];
			datamsg y(p, t, e);
			memcpy(buf2, &y, sizeof(datamsg));
			chan.cwrite(buf2, sizeof(datamsg));
			double reply2;
			chan.cread(&reply2, sizeof(double));
			received << ", " << reply2 << endl;
			received.flush();

			count++;
		}
		bimdc.close();
		received.close();
	}


	//Task 3:
	//Request files

	if (filename != "")
	{
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);

		__int64_t file_length;
		chan.cread(&file_length, sizeof(__int64_t));
		cout << "The length of " << fname << " is " << file_length << endl;


		string path = "received/" + filename;
		ofstream received(path);

		char* buf3 = new char[buff_cap];

		int i=0;
		int n = file_length/buff_cap;
		while (i < n)
		{
			filemsg* file_req = (filemsg*)buf2;
			file_req->offset = i*buff_cap;
			file_req->length = buff_cap;
			chan.cwrite(buf2, len);			
			chan.cread(buf3, file_req->length);
			received.write(buf3, buff_cap);
			received.flush();
			i++;
		}
		int r = file_length%buff_cap;
		if (r != 0)
		{
			filemsg* file_req = (filemsg*)buf2;
			file_req->offset = i*buff_cap;
			file_req->length = file_length%buff_cap;
			chan.cwrite(buf2, len);			
			chan.cread(buf3, file_req->length);
			received.write(buf3, file_length%buff_cap);
			received.flush();
		}

		received.close();
		delete[] buf2;
		delete[] buf3;
	}


	//Task 5:
	// Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}
