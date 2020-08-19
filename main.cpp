#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

using namespace std;

unsigned int max_buf_length=0;
unsigned int max_dict_length=0;
unsigned int cur_dict_length=0;
unsigned int cur_buf_length=0;

struct link {
    unsigned short length;
    unsigned short offset;
    unsigned char next;
};

struct Node
{
    Node* prev;
    unsigned int index;
};

class LinkedList
{
    public: Node* lastNode;

    LinkedList()
    {
        lastNode=nullptr;
    }

    ~LinkedList()
    {
        Node* temp;
        while(lastNode!=nullptr)
        {
            temp=lastNode;
            lastNode = lastNode->prev;
            delete temp;
        }
    }

    void PushBack(unsigned int val)
    {
        Node* myNode = new Node;
        myNode->index=val;
        myNode->prev=lastNode;
        lastNode=myNode;
    }
};

unsigned int readFile(unsigned char* &raw, fstream &inp)
{
    inp.seekg(0, ios::beg);
    unsigned int file_start = inp.tellg();
    inp.seekg(0, ios::end);
    unsigned int file_end = inp.tellg();
    unsigned int file_size = file_end - file_start;
    inp.seekg(0, ios::beg);
    raw = new unsigned char[file_size];
    inp.read((char*)raw, file_size);
    return file_size;
}

void FindLongestMatch(unsigned char* s, unsigned int buf_start, unsigned short &len, unsigned short &off, LinkedList dict[])
{
    Node* current = dict[s[buf_start]].lastNode;
    unsigned int max_offset = buf_start - cur_dict_length;
    unsigned int j = 0;
    unsigned int k = 0;
    if(current!=nullptr && (current->index)>=max_offset) { len=1; off=buf_start-(current->index); }
    while(current!=nullptr && (current->index)>=max_offset)
    {
       j=1;
       k=1;
       while(k<cur_buf_length && s[(current->index)+j]==s[buf_start+k])
       {
            if((current->index)+j==buf_start-1) { j=0; }
            else j++;
            k++;
       }
       if(k>len)
       {
            len = k;
            off = buf_start-(current->index);
            if(len==cur_buf_length) break;
       }
       else
       {
            current=current->prev;
       }
    }
}

int UpdateDictionary(unsigned char* s, unsigned int shift_start, unsigned short Length, LinkedList dict[])
{
    for(unsigned int i=shift_start; i<(shift_start+Length+1); i++)
    {
         dict[s[i]].PushBack(i);
    }
    return Length;
}

void compactAndWriteLink(link inp, vector<unsigned char> &out)
{
        if(inp.length!=0)
        {
            out.push_back((unsigned char)((inp.length << 4) | (inp.offset >> 8)));
            out.push_back((unsigned char)(inp.offset));
        }
        else
        {
            out.push_back((unsigned char)((inp.length << 4)));
        }
        out.push_back(inp.next);
}

void compressData(unsigned int file_size, unsigned char* data, fstream &file_out)
{
    LinkedList dict[256];
    link myLink;
    vector<unsigned char> lz77_coded;
    lz77_coded.reserve(2*file_size);
    bool hasOddSymbol=false;
    unsigned int target_size = 0;
    file_out.seekp(0, ios_base::beg);
    cur_dict_length = 0;
    cur_buf_length = max_buf_length;
    for(unsigned int i=0; i<file_size; i++)
    {
        if((i+max_buf_length)>=file_size)
        {
            cur_buf_length = file_size-i;
        }
        myLink.length=0;myLink.offset=0;
        FindLongestMatch(data,i,myLink.length,myLink.offset, dict);
        i=i+UpdateDictionary(data, i, myLink.length, dict);
        if(i<file_size) {
            myLink.next=data[i]; }
            else { myLink.next='_'; hasOddSymbol=true; }
        compactAndWriteLink(myLink, lz77_coded);
        if(cur_dict_length<max_dict_length) {
        while((cur_dict_length < max_dict_length) && cur_dict_length < (i+1)) cur_dict_length++;
        }
   }
   if(hasOddSymbol==true) { lz77_coded.push_back((unsigned char)0x1); }
   else lz77_coded.push_back((unsigned char)0x0);
   target_size=lz77_coded.size();
   file_out.write((char*)lz77_coded.data(), target_size);
   lz77_coded.clear();
   printf("Output file size: %d bytes\n", target_size);
   printf("Compression ratio: %.3Lf:1\n", ((double)file_size/(double)target_size));
}

void uncompressData(unsigned int file_size, unsigned char* data, fstream &file_out)
{
    if(file_size==0) { printf("Error! Input file is empty\n"); return; }
    link myLink;
    vector<unsigned char> lz77_decoded;
    lz77_decoded.reserve(4*file_size);
    unsigned int target_size = 0;
    unsigned int i=0;
    int k=0;
    file_out.seekg(0, ios::beg);
    while(i<(file_size-1))
    {
        if(i!=0) { lz77_decoded.push_back(myLink.next); }
        myLink.length = (unsigned short)(data[i] >> 4);
        if(myLink.length!=0)
        {
            myLink.offset = (unsigned short)(data[i] & 0xF);
            myLink.offset = myLink.offset << 8;
            myLink.offset = myLink.offset | (unsigned short)data[i+1];
            myLink.next = (unsigned char)data[i+2];
            if(myLink.offset>lz77_decoded.size()) {
             printf("Error! Offset is out of range!");
             lz77_decoded.clear();
             return;
             }
            if(myLink.length>myLink.offset)
            {
            k = myLink.length;
            while(k>0)
            {
                if(k>=myLink.offset)
                {
                lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end());
                k=k-myLink.offset;
                }
                else
                {
                lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end()-myLink.offset+k);
                k=0;
                }
            }
            }
            else lz77_decoded.insert(lz77_decoded.end(), lz77_decoded.end()-myLink.offset, lz77_decoded.end()-myLink.offset+myLink.length);
            i++;
        }
        else {
            myLink.offset = 0;
            myLink.next = (unsigned char)data[i+1];
        }
        i=i+2;
    }
    unsigned char hasOddSymbol = data[file_size-1];
    if(hasOddSymbol==0x0 && file_size>1) { lz77_decoded.push_back(myLink.next); }
    target_size = lz77_decoded.size();
    file_out.write((char*)lz77_decoded.data(), target_size);
    lz77_decoded.clear();
    printf("Output file size: %d bytes\n", target_size);
    printf("Unpacking finished!");
}

int main(int argc, char* argv[])
{
    max_buf_length=15; //16-1;
    max_dict_length=4095; //4096-1
    string filename_in ="";
    string filename_out="";
    fstream file_in;
    fstream file_out;
    unsigned int raw_size = 0;
    unsigned char* raw = nullptr;
    int mode = 0;
    printf("Simple LZ77 compression/decompression program\n");
    printf("Coded by MVoloshin, TSU, 2020\n");
    if(argc==4) {
        if(strcmp(argv[1], "-u")==0) mode = 0;
        else if(strcmp(argv[1], "-c")==0) mode = 1;
        else { printf("Unknown parameter, use -c or -u\n"); return 0; }
        filename_in=std::string(argv[2]);
        filename_out=std::string(argv[3]);
    }
    else
    {
        printf("Usage: [-c | -u] [input_file] [output_file]\n");
        return 0;
    }
    file_in.open(filename_in, ios::in | ios::binary);
    file_in.unsetf(ios_base::skipws);
    file_out.open(filename_out, ios::out);
    file_out.close();
    file_out.open(filename_out, ios::in | ios::out | ios::binary);
    file_out.unsetf(ios_base::skipws);
   if(file_in && file_out) {
   raw_size=readFile(raw, file_in);
   printf("Input file size: %d bytes\n", raw_size);
   }
   else
   {
        if(!file_in) printf("Error! Couldn't open input file!\n");
        if(!file_out) printf("Error! Couldn't open output file!\n");
        mode = -1;
   }
   file_in.close();
   if(mode==0)
   {
        uncompressData(raw_size, raw, file_out);
   }
   else if(mode==1)
   {
        compressData(raw_size, raw, file_out);
   }
   if(raw!=nullptr) delete[] raw;
   file_out.close();
    return 0;
}
