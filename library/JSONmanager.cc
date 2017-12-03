/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/ 

#include "JSONmanager.h"
#include <unistd.h>

JSONmanager::JSONmanager(int procId, unsigned long long options, vector<WKLDchar>*  WKLDcharForThreads){
    JSONwriter = std::unique_ptr<rapidjson::PrettyWriter<rapidjson::StringBuffer>>(new rapidjson::PrettyWriter<rapidjson::StringBuffer>(JSONbuffer));

    PISAfp = NULL;
    AddJSONfp = NULL;
    
    this->options = options;
    this->WKLDcharForThreads = WKLDcharForThreads;
    this->procId = procId;
}

void JSONmanager::openMaster(char* PISAFileName, char* AddJSONData, char* AppName, char* TestName){
    masterId = procId;

    /* Open PISA output file pointer */
    if(PISAFileName != NULL && strlen(PISAFileName) > 0){// Connect to the output file
        for(int trial = 0; trial<10 ; trial++){
            PISAfp = fopen(PISAFileName,"w");
        
            if(PISAfp)// Retry 10 times.
                break;
            else
                sleep(1);
        }
    }
    else
        PISAfp = stderr;
  
    if(PISAfp == NULL){
        fprintf(stderr,"JSONman: Error opening PISA output file: %s\n",PISAFileName);
        exit(1);
    }

    /* Open input file with additional JSON data */
    if(AddJSONData != NULL && strlen(AddJSONData) > 0){// Connect to the output file
        for(int trial = 0; trial<10 ; trial++){
            AddJSONfp = fopen(AddJSONData,"r");
            if(AddJSONfp)// Retry 10 times.
                break;
            else
                sleep(1);
        }
    }

    dump_JSON_header(JSONwriter.get(),AppName,TestName);// Prepare JSON header.
}

void JSONmanager::openSlave(int masterId){
    this->masterId = masterId;
    PISAfp = NULL;
}


#define RECV_JSON_COMMAND() \
    MPI_Recv(&jsonCommand, 1, MPI_INT, sourceId, 0, MPI_COMM_WORLD, &status);

#define SEND_JSON_COMMAND(command) \
    MPI_Ssend(&command, 1, MPI_INT, masterId, 0, MPI_COMM_WORLD); 

#define DEBUG_JSON_COMM 0
void JSONmanager::dump(int procId){
    if(this->procId == procId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: process %d dumping JSON\n",procId);
        #endif

        unsigned long long sharedBytes = 0;
        unsigned long long sharedAccesses = 0;

        compute_shared_memory((*WKLDcharForThreads), options, sharedBytes, sharedAccesses);

        if(procId != masterId){
            unsigned long long threads = WKLDcharForThreads->size();
            #if DEBUG_JSON_COMM
                fprintf(stderr,"JSONman: Slave considers %llu threads\n", threads);
            #endif
            MPI_Ssend(&threads, 1, MPI_UNSIGNED_LONG_LONG, masterId, 0, MPI_COMM_WORLD);// communicate the number of threads
        }
        
        for (unsigned long i = 0; i < WKLDcharForThreads->size(); i++) {
          if (InitializedThreads[i]) {
              (*WKLDcharForThreads)[i].JSONdump(this, sharedBytes, sharedAccesses);
          }

            if(procId != masterId){
                int command = JSON_END_THREAD;
                SEND_JSON_COMMAND(command);// communicate the thread data terminated
            }
        }
    }

    else if(PISAfp != NULL){// This is the master
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master collecting JSON for %d\n",procId);
        #endif

        dumpFrom(procId);
    }
}

void JSONmanager::dumpFrom(int sourceId){
    MPI_Status status;

    unsigned long long threads = 0;
    MPI_Recv(&threads, 1, MPI_UNSIGNED_LONG_LONG, sourceId, 0, MPI_COMM_WORLD, &status);
    #if DEBUG_JSON_COMM
        fprintf(stderr,"JSONman: Master expects %llu threads\n", threads);
    #endif


    for (unsigned long i = 0; i < threads; i++) {
        int jsonCommand;

        RECV_JSON_COMMAND();
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master starts managing command %d\n", jsonCommand);
        #endif
        while(jsonCommand != JSON_END_THREAD){

            switch(jsonCommand){

                case JSON_START_ARRAY:
                    StartArray();
                break;

                case JSON_END_ARRAY:
                    EndArray();
                break;

                case JSON_START_OBJECT:
                    StartObject();
                break;

                case JSON_END_OBJECT:
                    EndObject();
                break;

                case JSON_ULL:
                    unsigned long long ULLdatum;
                MPI_Recv(&ULLdatum, 1, MPI_UNSIGNED_LONG_LONG, sourceId, 0, MPI_COMM_WORLD, &status);
                    Uint64(ULLdatum);
                break;

                case JSON_STRING:
                    char StringDatum[JSON_MAX_STRING_LENGTH];
                    int recvLength, actLength;

                    MPI_Recv(&recvLength, 1, MPI_INT, sourceId, 0 , MPI_COMM_WORLD, &status);

                    #if DEBUG_JSON_COMM
                        fprintf(stderr,"JSONman: Master expecting %d chars\n", recvLength);
                    #endif

                MPI_Recv(StringDatum, recvLength, MPI_CHAR, sourceId, 0, MPI_COMM_WORLD, &status);

                    #if DEBUG_JSON_COMM
                        fprintf(stderr,"JSONman: Master received: %s\n", StringDatum);
                    #endif
                    String(StringDatum);
                break;

                case JSON_DOUBLE:
                    double doubleDatum;
                MPI_Recv(&doubleDatum, 1, MPI_DOUBLE, sourceId, 0, MPI_COMM_WORLD, &status);
                    Double(doubleDatum);
                break;

                default:
                    fprintf(stderr,"JSONman: Error: unknown JSON command to be managed\n");
                break;
            }

            RECV_JSON_COMMAND();
            #if DEBUG_JSON_COMM
                fprintf(stderr,"JSONman: Master starts managing command %d\n", jsonCommand);
            #endif
        }
        // Received thread end command
    }
    // Received all thread data for the process sourceId
}

void JSONmanager::StartArray(){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for StartArray()\n");
        #endif
        int command = JSON_START_ARRAY;
        SEND_JSON_COMMAND(command);

    } else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing StartArray()\n");
        #endif
        JSONwriter -> StartArray();
    }
}

void JSONmanager::EndArray(){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for EndArray()\n");
        #endif
        int command = JSON_END_ARRAY;
        SEND_JSON_COMMAND(command);

    } else{ // masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing EndArray()\n");
        #endif
        JSONwriter -> EndArray();
    }
}

void JSONmanager::StartObject(){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for StartObject()\n");
        #endif
        int command = JSON_START_OBJECT;
        SEND_JSON_COMMAND(command);

    }else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing StartObject()\n");
        #endif
        JSONwriter -> StartObject();
    }
}

void JSONmanager::EndObject(){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for EndObject()\n");
        #endif
        int command = JSON_END_OBJECT;
        SEND_JSON_COMMAND(command);

    }else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing EndObject()\n");
        #endif
        JSONwriter -> EndObject();
    }
}

void JSONmanager::Uint64(unsigned long long datum){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for Uint64(%llu)\n",datum);
        #endif
        int command = JSON_ULL;
        SEND_JSON_COMMAND(command);
        MPI_Ssend(&datum, 1, MPI_UNSIGNED_LONG_LONG, masterId, 0, MPI_COMM_WORLD);

    }else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing Uint64(%llu)\n",datum);
        #endif
        JSONwriter -> Uint64(datum);
    }
}

void JSONmanager::String(const char* datum){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for String(\"%s\")\n",datum);
        #endif
        int command = JSON_STRING;
        int length = strlen(datum)+1;
        SEND_JSON_COMMAND(command);
        MPI_Ssend(&length, 1, MPI_INT, masterId, 0, MPI_COMM_WORLD);
        MPI_Ssend(datum, strlen(datum)+1, MPI_CHAR, masterId, 0, MPI_COMM_WORLD);

    }else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing String(\"%s\")\n",datum);
        #endif
        JSONwriter -> String(datum);
    }
}

void JSONmanager::Double(double datum){
    if(procId != masterId){
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Slave messaging for Double(%f)\n",datum);
        #endif
        int command = JSON_DOUBLE;
        SEND_JSON_COMMAND(command);
        MPI_Ssend(&datum, 1, MPI_DOUBLE, masterId, 0, MPI_COMM_WORLD);

    }else{// masterThread
        #if DEBUG_JSON_COMM
            fprintf(stderr,"JSONman: Master managing Double(%f)\n",datum);
        #endif
        JSONwriter -> Double(datum);
    }
}

void JSONmanager::close(){
    if(PISAfp != NULL){// The master needs to write the footer and closes the file.
        dump_JSON_footer(JSONwriter.get(),&JSONbuffer,PISAfp);
        if(PISAfp != stderr)
            fclose(PISAfp);
    }

    if(AddJSONfp != NULL){// The master needs to write the footer and closes the file.
        fclose(AddJSONfp);
    }
}

void currentDateTime(char *buf) {
    time_t now = time(0);
    struct tm tstruct;
    tstruct = *localtime(&now);
    strftime(buf, 80, "%FT%TZ", &tstruct);
}

string getCmdExe() {
    char cmd[100];
    pid_t threadPid = getpid();
    sprintf(cmd, "cat /proc/%d/cmdline | tr \"\\0\" \" \"", threadPid);

    char buf[1024];
    string result;

    FILE *out;
    out = popen(cmd, "r");
    while (fgets(buf, 1024, out) != NULL)
        result.append(buf);
    pclose(out);

    return result;
}


void JSONmanager::dump_JSON_header(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter,char* AppName,char* TestName) {
    JSONwriter->StartObject();

    bool hasApp,hasScenario,hasExecmd,hasTime;
    hasApp = false;
    hasScenario = false;
    hasExecmd = false;
    hasTime = false;

    // Dump additional data
    dump_additional_data(JSONwriter,hasApp,hasScenario,hasExecmd,hasTime);

    // Dump the default values
    if(!hasApp){
        JSONwriter->String("application");
        JSONwriter->String(AppName);
    }

    if(!hasScenario){
        JSONwriter->String("test-scenario");
        JSONwriter->String(TestName);
    }

    if(!hasExecmd){
        JSONwriter->String("execmd");
        string cmdExe = getCmdExe();
        JSONwriter->String(cmdExe.c_str());
    }

    if(!hasTime){
        char currentTime[80];
        JSONwriter->String("time");
        currentDateTime(currentTime);
        JSONwriter->String(currentTime);
    }

    // Dump the threads
    JSONwriter->String("threads");
    JSONwriter->StartArray();
}

void JSONmanager::dump_additional_object(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter, rapidjson::Value* obj){
    for (rapidjson::Value::ConstMemberIterator itr = obj->MemberBegin(); itr != obj->MemberEnd(); ++itr){
        JSONwriter->String( itr->name.GetString() );
    
        if(itr->value.IsInt()){
            JSONwriter->Int(itr->value.GetInt());
        }

        if(itr->value.IsDouble()){
            JSONwriter->Double(itr->value.GetDouble());
        }

        if(itr->value.IsString()){
            JSONwriter->String(itr->value.GetString());
        }

        if(itr->value.GetType() == rapidjson::kObjectType){
            JSONwriter->StartObject();
            dump_additional_object(JSONwriter, (rapidjson::Value*) &(itr->value));
            JSONwriter->EndObject();
        }

        if(itr->value.GetType() == rapidjson::kArrayType)
            fprintf(stderr,"Warning: array type not supported as additional data. The final JSON will not include: %s\n",itr->name.GetString());
    }
}

void JSONmanager::dump_additional_data(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter,
                                       bool& hasApp, bool& hasScenario, bool& hasExecmd, bool& hasTime){

    if(AddJSONfp != NULL){ // There is additional data to carry on!
        char readBuffer[65536];
        rapidjson::FileReadStream is(AddJSONfp, readBuffer, sizeof(readBuffer));
        rapidjson::Document document;
        document.ParseStream(is);

        if(document.HasMember("application"))
            hasApp = true;
        if(document.HasMember("test-scenario"))
            hasScenario = true;
        if(document.HasMember("execmd"))
            hasExecmd = true;
        if(document.HasMember("time"))
            hasTime = true;
        
        dump_additional_object(JSONwriter, &document);
    }
}

void JSONmanager::dump_JSON_footer(rapidjson::PrettyWriter<rapidjson::StringBuffer> *JSONwriter, 
                                   rapidjson::StringBuffer* JSONbuffer, FILE* PISAfp) {

    // end threads array
    JSONwriter->EndArray();
  
    // end all JSON
    JSONwriter->EndObject();

    fprintf(PISAfp,"%s\n",JSONbuffer->GetString());
    // JSONbuffer->Clear();
}

