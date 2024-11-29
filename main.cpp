#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include "stdlib.h"

//https://man7.org/linux/man-pages/man2/posix_fadvise.2.html

int size = 512;

int posix_write(const char* in, const char *out, const bool nobuf = false){
    int flag = O_CREAT | O_WRONLY;
    if(nobuf) flag |= O_SYNC | O_DIRECT;
    auto fout = open(out,  flag, 0777); //O_SYNC | O_DIRECT
    auto fin = open(in, O_RDONLY);
    
    auto buff = new uint8_t[size];

    if(!fin || !fout){
        std::cout << "file open error \n";
        perror("open");
        goto err;
    }
    
    for(auto bytes = read(fin, buff, size); 
            bytes > 0; 
            bytes = read(fin, buff, size)){
        write(fout, buff, bytes);
    }
    close(fin);
    fdatasync(fout);
    close(fout);
    delete [] buff;
    return 0;
    err:
        close(fin);
        close(fout);
        delete [] buff;
        return -1;
}

int c_write(const char* in, const char *out, const bool usePosix = false,const bool nobuf = false){
    auto fin = fopen(in, "r");
    auto fout = fopen(out, "w");
    int fd = fileno(fout);
//     auto outb = new char[size];
    bool ret = true;
    long fsize = 0;
    char* inb = 0;
    
    int partSize = size;
    int bytes = 0;
    int written = 0;
    
    if(!fin || !fout){
        std::cout << "error open file\n";
        goto err;
    }

//     if(nobuf)
//         ret &= !bool(setvbuf(fout,0,_IONBF,0));
//     else 
//         ret &= !bool(setvbuf(fout,outb,0,size));
    if(!ret) goto err;

    fseek(fin, 0, SEEK_END);
    fsize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    inb = new char[fsize];
    ret &= std::fread(inb, fsize, 1, fin) != 0;
    if(!ret){
        std::cout << "read error " << std::ferror(fin) << '\n';
        goto err; 
    }
    if(nobuf){ //this make flash writing OK
    while( bytes < fsize && partSize > 0){
        if(!usePosix) written = std::fwrite(inb + bytes, 1, partSize, fout);
        else written = write(fd, inb+bytes, partSize);
        if(written > 0)
            bytes+= written;
        else {
            std::cout << "write error\n";
            goto err;
        }
//         printf("bytes %d part %d\n",bytes, partSize);
        if((bytes + partSize) > fsize) partSize = fsize - bytes;
        
    }
    }
    else{
    ret &= std::fwrite(inb, fsize, 1, fout) != 0;
    if(!ret){
        std::cout << "write error\n";
        goto err; 
    }
    }
    fclose(fin);
    std::fflush(fout);
    fclose(fout);
    delete [] inb;
//     delete [] outb;
    return 0;
    err:
        fclose(fin);
        fclose(fout);
        delete [] inb;
//         delete [] outb;
        return -1;
}

int fstream_write(const char* in, const char *out, const bool nobuf = false){
    auto fin = std::fstream(in, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);//ate = open at the end
    auto fout = std::fstream(out, std::ios_base::out | std::ios_base::binary);
    if(!fin.is_open() || !fout.is_open()) {
        std::cout << "error file open\n";
        return -1;
    }
    std::string buf;
    buf.resize(fin.tellg());
    fin.seekg(0);
    fin.read((char*)buf.data(), buf.size());
    fout.write(buf.data(), buf.size());
    fout.flush();
    return 0;
}

int main(int argc, const char ** argv){
    std::string in, out;
    std::string mode;
    if(argc > 1)
        in = argv[1];
    if(argc > 2)
        out = argv[2];
    if(argc > 4){
        if(!strcmp(argv[3], "-m")){
            mode = argv[4];
        }
        else {
            std::cout << "undefined option " << argv[3] << " " << argv[4] << '\n';
        }
    }
    if(argc > 5){
        size = atoi(argv[5]);
        
    }
    std::cout << "in " << in << '\n';
    std::cout << "out " << out << '\n';
    std::cout << "mode " << mode << '\n';
    std::cout << "buffsize " << size << '\n';
    if(in.empty() || out.empty()) {
        std::cout << "empty filenames\n";
        return -2;
    }

    
    const bool nobuf = mode.find("nb") != std::string::npos;
    if(nobuf) std::cout << "without buffer\n";
    if(mode.find("cp") != std::string::npos){
        std::cout << "C with posix write\n";
        return c_write(in.c_str(), out.c_str(),true, nobuf);
    }
    else if(mode.find('c') != std::string::npos){
        return c_write(in.c_str(), out.c_str(), nobuf);
    }
    else if(mode.find('f') != std::string::npos) {
        return fstream_write(in.c_str(), out.c_str(), nobuf);
    }
    else {
        return posix_write(in.c_str(), out.c_str(), nobuf);
    }
    return 0;
}
