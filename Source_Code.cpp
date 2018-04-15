//============================================================================
// Name        : MSD.cpp
// Author      : Manjush PV, Ritwik MB, Suprita K, Vadiraja MN
// Version     : 0.0.0000
// Copyright   : @c to our own group
// Description : MSD Project : L1 split cache design
//============================================================================

#include <iostream>
#include<fstream>
#include <math.h>
#include <string>
#include<regex>

using namespace std;

//Data Cache Specifications
int const DC_Associativity = 4;
int const DC_Sets = 16384;
int const DC_Byte_Select = 64;

//Instruction Cache Specifications
int const IC_Associativity = 2;
int const IC_Sets = 16384;
int const IC_Byte_Select = 64;

int const address_Length = 32;
int MESI_Size = 1;
int mode;

//Cache Line Declarations
struct Data_Cache_Line
{
    string tag_Bits = "";
    string MESI_Bits = "";
    int LRU_Bits = 0;
};

struct Instruction_Cache_Line
{
    string tag_Bits = "";
    string MESI_Bits = "";
    int LRU_Bits = 0;
};

//Cache Memory Declarations
Data_Cache_Line Data_Cache[DC_Sets][DC_Associativity];
Instruction_Cache_Line Instruction_Cache[IC_Sets][IC_Associativity];

//Cache Types for L1 split Cache
struct Cache_Type
{
public:
    string Data = "Data";
    string Instruction = "Instruction";
};

//Statistics parameter declarations for both I and D Caches
struct Statistics
{
public:
    long long Data_Read_Count = 0;
    long long Data_Write_Count = 0;
    long long Data_Hit_Count = 0;
    long long Data_Miss_Count = 0;
    double Data_Hit_Ratio = 0;
    
    long long Instruction_Read_Count = 0;
    long long Instruction_Hit_Count = 0;
    long long Instruction_Miss_Count = 0;
    double Instruction_Hit_Ratio = 0;
};

//Cache Line and Address Parameters Size
struct Data_Cache_Parameters
{
public:
    int byte_Select = 0;
    int index_Size = 0;
    int tag_Size = 0;
    int LRU_Size = 0;
};

struct Instruction_Cache_Parameters
{
    int byte_Select = 0;
    int index_Size = 0;
    int tag_Size = 0;
    int LRU_Size = 0;
};

//Computing Address and Cache Line Parameters Size
class Address_Cache_Line_Size
{
public: Address_Cache_Line_Size(){};
    
public:
    int byte_Select,index_Size,tag_Size,no_LRU_bits;
    
    void cache_Parameters(int address_Length, int cache_Line_Size, int no_Sets, int associativity)
    {
        byte_Select=log2(cache_Line_Size);
        index_Size=log2(no_Sets);
        tag_Size = address_Length - (index_Size + byte_Select);
        no_LRU_bits=log2(associativity);
    }
};

//Bifurcating Address into Tag, Index and Byte_Select
//This contains all conversions --> Hexa to Integer, Binary to Hexa, Binary to Integer, Integer to Binary, Zeroes appending to Binary, Hexa to Binary
class Address_Bifurcation
{
public: string tag_Bits,index_Bits,byte_Select_Bits;
    long int_Index;
    
public:const char* getBinaryForAlphabet(char alphabet)
    {
        switch(alphabet)
        {
            case 'a':
                return "10";
            case 'b':
                return "11";
            case 'c':
                return "12";
            case 'd':
                return "13";
            case 'e':
                return "14";
            case 'f':
                return "15";
        }
        return "-1";
    }
    
public:long convertHexaToInteger(string hexadecimal)
    {
        long intValue = 0;
        long tempInteger;
        
        for(int i = 0;i<hexadecimal.size();i++)
        {
            if(hexadecimal[i]>='A'&&hexadecimal[i]<='F')
            {
                tempInteger=atoi(getBinaryForAlphabet(hexadecimal[i]+32));
            }
            else
            {
                if(hexadecimal[i]>='a'&&hexadecimal[i]<='f')
                {
                    tempInteger=atoi(getBinaryForAlphabet(hexadecimal[i]));
                }
                else
                {
                    tempInteger=(int)hexadecimal[i]-'0';
                }
            }
            intValue += tempInteger * (pow(16,hexadecimal.size()-1-i));
        }
        return intValue;
    }
    
public:string convertBinaryToHex(string binaryValue)
    {
        string hexValue = "";
        const char *hex_dig = "0123456789abcdef";
        const char *multiplier = "8421";
        int append_zeroes =4 - (binaryValue.size()%4);
        if(binaryValue.size()%4 != 0)
        {
            for(int i=0;i<append_zeroes;i++)
                binaryValue ="0"+binaryValue;
        }
        int intNibble = 0;
        string tempNibble = "";
        
        for(int i=0;i<binaryValue.size()/4;i++)
        {
            for(int j=0;j<4;j++)
            {
                intNibble += (int)(binaryValue[(i*4)+j] - '0')*(int)(multiplier[j] - '0');
            }
            tempNibble = hex_dig[intNibble];
            hexValue += tempNibble;
            intNibble = 0;
        }
        return hexValue;
    }
    
public:long convertBinaryToInteger(string binaryValue)
    {
        long intValue = 0;
        
        for(int i=0;i<binaryValue.size();i++)
        {
            intValue += (int)(binaryValue[i] - '0') * pow(2,(binaryValue.size()-1-i));
        }
        return intValue;
    }
    
public:string convertIntegerToBinary(long intValue)
    {
        string binaryValue;
        if(intValue == 0)
            binaryValue = "0";
        while(intValue != 0)
        {
            binaryValue = to_string(intValue % 2) + binaryValue;
            intValue = intValue/2;
        }
        return binaryValue;
    }
    
public:string appendZeroesToBinary(string binaryValue, int size)
    {
        for(unsigned int j=(int)binaryValue.size();j<size;j++)
        {
            binaryValue="0"+binaryValue;
        }
        return binaryValue;
    }
public:string convertHexaToBinary(string hexadecimal)
    {
        return convertIntegerToBinary(convertHexaToInteger(hexadecimal));
    }
    
public:void bifurcateAddress(string hexadecimal,int index_Size, int address_Length, int tag_Size,int byte_Select)
    {
        string binary_Hex;
        binary_Hex = this->convertHexaToBinary(hexadecimal);
        
        for(int k=(int)binary_Hex.size();k<address_Length;k++)
        {
            binary_Hex="0"+binary_Hex;
        }
        
        tag_Bits=binary_Hex.substr(0,tag_Size);
        index_Bits=binary_Hex.substr(tag_Size,index_Size);
        byte_Select_Bits=binary_Hex.substr(tag_Size+index_Size,byte_Select);
        int_Index = this->convertBinaryToInteger(index_Bits);
    }
};

//Checks for Cache Status --> if its a hit or miss
//                        --> if the cache set is full or halfway filled
//                        --> getting the Least recently used cache line in a set --> for eviction of a cache line in a fully filled set
class CacheSetStatus
{
public: int way; string tag;
    
public: bool hit(long set, int associativity, string tag, string cache_Type)
    {
        string temp_Tag;
        string temp_MESI;
        bool hit_Status = false;
        for (int i=0; i<associativity; i++)
        {
            if (cache_Type == "Data")
            {
                temp_Tag = Data_Cache[set][i].tag_Bits;
                temp_MESI = Data_Cache[set][i].MESI_Bits;
            }
            else
            {
                temp_Tag = Instruction_Cache[set][i].tag_Bits;
                temp_MESI = Instruction_Cache[set][i].MESI_Bits;
            }
            if (temp_Tag == tag && temp_MESI != "I")
            {
                hit_Status = true;
                way = i;
                break;
            }
            else
                way = -1;
        }
        return hit_Status;
    }
    
public: bool invalidLine(long set, int associativity, string cache_Type)
    {
        string temp_MESI;
        bool empty_line_Status = false;
        for (int i=0; i<associativity; i++)
        {
            if (cache_Type == "Data")
                temp_MESI = Data_Cache[set][i].MESI_Bits;
            else
                temp_MESI = Instruction_Cache[set][i].MESI_Bits;
            if (temp_MESI == "I")
            {
                empty_line_Status = true;
                way = i;
                break;
            }
            else
                way = -1;
        }
        return empty_line_Status;
    }
    
public: void highestLRU(long set, int associativity, string cache_Type)
    {
        int temp_LRU_Bits;
        for (int i=0; i<associativity; i++)
        {
            if (cache_Type == "Data")
                temp_LRU_Bits = Data_Cache[set][i].LRU_Bits;
            else
                temp_LRU_Bits = Instruction_Cache[set][i].LRU_Bits;
            if (temp_LRU_Bits == (associativity - 1))
            {
                way = i;
                break;
            }
            else
                way = -1;
        }
        if (cache_Type == "Data")
            tag = Data_Cache[set][way].tag_Bits;
        else
            tag = Instruction_Cache[set][way].tag_Bits;
    }
    
};

//LRU Update for different scenarios
// On Hit
// On Miss --> when set is not full
//         --> when set is full
//Update LRU for the set when there is an invalidate to a line in that set

class LRU_Bits_Update
{
    
public: void LRU_Update_Hit(long set, int associativity, string cache_Type, int way)
    {
        int temp_LRU_Bits,hit_LRU_Bits;
        string temp_MESI;
        
        if (cache_Type == "Data")
            hit_LRU_Bits = Data_Cache[set][way].LRU_Bits;
        else
            hit_LRU_Bits = Instruction_Cache[set][way].LRU_Bits;
        
        for (int i=0; i<associativity; i++)
        {
            if (cache_Type == "Data")
            {
                temp_LRU_Bits = Data_Cache[set][i].LRU_Bits;
                temp_MESI = Data_Cache[set][i].MESI_Bits;
            }
            else
            {
                temp_LRU_Bits = Instruction_Cache[set][i].LRU_Bits;
                temp_MESI = Instruction_Cache[set][i].MESI_Bits;
            }
            
            if (temp_LRU_Bits < hit_LRU_Bits && temp_MESI != "I")
                temp_LRU_Bits++;
            
            if (cache_Type == "Data")
                Data_Cache[set][i].LRU_Bits = temp_LRU_Bits;
            else
                Instruction_Cache[set][i].LRU_Bits = temp_LRU_Bits;
        }
        
        if (cache_Type == "Data")
            Data_Cache[set][way].LRU_Bits = 0;
        else
            Instruction_Cache[set][way].LRU_Bits = 0;
    }
    
public: void LRU_Update_Miss(long set, int associativity, string cache_Type, int way)
    {
        int temp_LRU_Bits;
        string temp_MESI;
        
        if (cache_Type == "Data")
            Data_Cache[set][way].LRU_Bits = 0;
        else
            Instruction_Cache[set][way].LRU_Bits = 0;
        
        for (int i=0; i<associativity; i++)
        {
            if (cache_Type == "Data")
            {
                temp_LRU_Bits = Data_Cache[set][i].LRU_Bits;
                temp_MESI = Data_Cache[set][i].MESI_Bits;
            }
            else
            {
                temp_LRU_Bits = Instruction_Cache[set][i].LRU_Bits;
                temp_MESI = Instruction_Cache[set][i].MESI_Bits;
            }
            
            if (i!= way && temp_MESI != "I")
                temp_LRU_Bits++;
            
            if (cache_Type == "Data")
                Data_Cache[set][i].LRU_Bits = temp_LRU_Bits;
            else
                Instruction_Cache[set][i].LRU_Bits = temp_LRU_Bits;
        }
    }
    
public: void LRU_Update_Invalidate(long set, int associativity, int way)
    {
        int temp_LRU_Bits,hit_LRU_Bits;
        string temp_MESI;
        
        hit_LRU_Bits = Data_Cache[set][way].LRU_Bits;
        
        for (int i=0; i<associativity; i++)
        {
            temp_LRU_Bits = Data_Cache[set][i].LRU_Bits;
            temp_MESI = Data_Cache[set][i].MESI_Bits;
            
            if (temp_LRU_Bits > hit_LRU_Bits && temp_MESI != "I")
                temp_LRU_Bits--;
            Data_Cache[set][i].LRU_Bits = temp_LRU_Bits;
        }
    }
};

// MESI states Transitions --> updating MESI
//                         --> clear MESI
//                         --> check for the current state of a cache line
class StateTransitions
{
public: void updateMESI(string MESI, string cache_Type,long set,int way)
    {
        if (cache_Type == "Data")
            Data_Cache[set][way].MESI_Bits = MESI;
        else
            Instruction_Cache[set][way].MESI_Bits = MESI;
    }
    
public: bool checkState(string MESI, string cache_Type, long set, int way)
    {
        bool status = false;
        if (cache_Type == "Data")
        {
            if (Data_Cache[set][way].MESI_Bits == MESI)
                status = true;
            else status = false;
        }
        return status;
    }
};

//Cache updates : --> reset cache to its initial state
//                --> updating Tag bits of cache line
//                --> reset Statistics
class CacheUpdate
{
public: void initializeCache(long set, int associativity, string cache_Type)
    {
        for (int i=0; i<set; i++)
        {
            for (int j=0; j<associativity; j++)
            {
                if (cache_Type == "Data")
                    Data_Cache[i][j].MESI_Bits = "I";
                else
                    Instruction_Cache[i][j].MESI_Bits = "I";
            }
        }
    }
    
public: void updateTag(int way, string tag, string cache_Type, long set, int tag_Size)
    {
        if (cache_Type == "Data")
            Data_Cache[set][way].tag_Bits = Data_Cache[set][way].tag_Bits.replace(0, tag_Size, tag);
        else
            Instruction_Cache[set][way].tag_Bits = Instruction_Cache[set][way].tag_Bits.replace(0, tag_Size, tag);
    }
    
public: Statistics clearStatistics ()
    {
        Statistics stats;
        stats.Data_Read_Count = 0;
        stats.Data_Write_Count = 0;
        stats.Data_Hit_Count = 0;
        stats.Data_Miss_Count = 0;
        stats.Data_Hit_Ratio = 0;
        
        stats.Instruction_Read_Count = 0;
        stats.Instruction_Hit_Count = 0;
        stats.Instruction_Miss_Count = 0;
        stats.Instruction_Hit_Ratio = 0;
        return stats;
    }
};

//Display Functions
//-->For Cache Contents
//-->For Statistics
//function to check if cache set is empty or not--> to display cache contents in tabular format
class Display_Contents
{
public: void Display_Cache_Contents(int sets, int associativity,string cache_Type)
    {
        Address_Bifurcation obj_Address_Bifurcation = Address_Bifurcation();
        for (int i=0; i<sets; i++)
        {
            bool set_display = false;
            for (int j=0; j<associativity; j++)
            {
                if (cache_Type == "Data")
                {
                    if (!set_display && Data_Cache[i][0].MESI_Bits == "I")
                    {
                        if (Set_Empty_Status("Data", associativity, i))
                            break;
                        else
                        {
                            set_display = true;
                            cout<<i;
                        }
                    }
                    if (Data_Cache[i][j].MESI_Bits != "I")
                    {
                        if(!set_display)
                            cout<<i<<"\t";
                        
                        cout << Data_Cache[i][j].MESI_Bits << " "<< Data_Cache[i][j].LRU_Bits << " " << obj_Address_Bifurcation.convertBinaryToHex(Data_Cache[i][j].tag_Bits) << "\t\t";
                        set_display = true;
                    }
                    else if(set_display)
                    {
                        cout<<"\t\t\t";
                    }
                }
                else
                {
                    if (!set_display && Instruction_Cache[i][0].MESI_Bits == "I")
                    {
                        if (Set_Empty_Status("Instruction", associativity, i))
                            break;
                        else
                        {
                            set_display = true;
                            cout<<i;
                        }
                    }
                    if (Instruction_Cache[i][j].MESI_Bits != "I")
                    {
                        if(!set_display)
                            cout<<i<<"\t";
                        cout << Instruction_Cache[i][j].MESI_Bits << " "<< Instruction_Cache[i][j].LRU_Bits << " " << obj_Address_Bifurcation.convertBinaryToHex(Instruction_Cache[i][j].tag_Bits) << "\t\t";
                        set_display = true;
                    }
                    else if(set_display)
                    {
                        cout<<"\t\t\t";
                    }
                }
            }
            if (set_display)
                cout<<endl;
        }
    }
    
public: void Display_Statistics(long long Data_Read_Count, long long Data_Write_Count, long long Data_Hit_Count, long long Data_Miss_Count, double Data_Hit_ratio, long long Instruction_Read_Count,  long long Instruction_Hit_Count, long long Instruction_Miss_Count, double Instruction_Hit_ratio)
    {
        cout << "Data Cache Statistics: \n" << "Read Count:" << Data_Read_Count << "\tWrite Count:" << Data_Write_Count << "\tHit Count:" << Data_Hit_Count << "\tMiss Count:" << Data_Miss_Count << "\tHit Ratio:" << Data_Hit_ratio << endl<<endl;
        
        cout << "Instruction Cache Statistics: \n" << "Read Count:" << Instruction_Read_Count << "\tHit Count:" << Instruction_Hit_Count << "\tMiss Count:" << Instruction_Miss_Count << "\tHit Ratio:" << Instruction_Hit_ratio << endl<<endl;
        
    }
    
public: bool Set_Empty_Status(string cache_Type, int associativity, int set)
    {
        bool status = false;
        int count = 0;
        for (int m=0; m<associativity; m++)
        {
            if (cache_Type == "Data")
            {
                if (Data_Cache[set][m].MESI_Bits == "I")
                    count++;
            }
            else
            {
                if (Instruction_Cache[set][m].MESI_Bits == "I")
                    count++;
            }
        }
        if (count == associativity)
            status = true;
        return status;
    }
};

//Computation of Address of a Cache Line when its been accessed, where Byte Select bits will be 0's
class GetAddress
{
public: string lineAddress(string tag, long int_index, int byte_select_size, int index_size)
    {
        string address, index;
        Address_Bifurcation obj_Address_Bifurcation = Address_Bifurcation();
        index = obj_Address_Bifurcation.convertIntegerToBinary(int_index);
        index = obj_Address_Bifurcation.appendZeroesToBinary(index, index_size);
        address = tag + index;
        address.insert(address.end(), byte_select_size, '0');
        return obj_Address_Bifurcation.convertBinaryToHex(address);
    }
};

//Display Messages
class DisplayMessages
{
public: string GetMessage(int value, string Address)
    {
        string Message;
        switch (value)
        {
            case 1:
                Message = "Return data to L2 "+Address;
                break;
            case 2:
                Message = "Write to L2 "+Address;
                break;
            case 3:
                Message = "Read from L2 "+Address;
                break;
            case 4:
                Message = "Read for Ownership from L2 "+Address;
                break;
            default:
                Message = "Invalid Message";
                break;
        }
        return Message;
    }
};

//Entry Point
int main(int argc, char *argv[])
{
    ifstream file;
    bool status;
    int way;
    regex reg("[g-zG-Z]+|(.+[g-zG-Z])");
    
    //Object Creation of classes declared above for computation of all the functionalities specified above
    CacheUpdate obj_CacheUpdate = CacheUpdate();
    Address_Cache_Line_Size obj_Address_Cache_Line_Size=Address_Cache_Line_Size();
    Address_Bifurcation obj_Address_Bifurcation=Address_Bifurcation();
    Instruction_Cache_Line ICL;
    Data_Cache_Line DCL;
    Statistics stats;
    CacheSetStatus obj_CacheSetStatus = CacheSetStatus();
    StateTransitions obj_StateTransitions = StateTransitions();
    Cache_Type CT;
    LRU_Bits_Update obj_LRU_Bits_Update = LRU_Bits_Update();
    DisplayMessages obj_DisplayMessages = DisplayMessages();
    Display_Contents obj_Display_Contents = Display_Contents();
    GetAddress obj_GetAddress = GetAddress();
    
    //Size Specs For Data Cache
    obj_Address_Cache_Line_Size.cache_Parameters(address_Length,DC_Byte_Select,DC_Sets,DC_Associativity);
    Data_Cache_Parameters DCP;
    DCP.byte_Select = obj_Address_Cache_Line_Size.byte_Select;
    DCP.index_Size = obj_Address_Cache_Line_Size.index_Size;
    DCP.tag_Size = obj_Address_Cache_Line_Size.tag_Size;
    DCP.LRU_Size = obj_Address_Cache_Line_Size.no_LRU_bits;
    
    //Size Specs For Instruction Cache
    obj_Address_Cache_Line_Size.cache_Parameters(address_Length,IC_Byte_Select,IC_Sets,IC_Associativity);
    Instruction_Cache_Parameters ICP;
    ICP.byte_Select = obj_Address_Cache_Line_Size.byte_Select;
    ICP.index_Size = obj_Address_Cache_Line_Size.index_Size;
    ICP.tag_Size = obj_Address_Cache_Line_Size.tag_Size;
    ICP.LRU_Size = obj_Address_Cache_Line_Size.no_LRU_bits;
    
    //Initialising Cache
    obj_CacheUpdate.initializeCache(DC_Sets, DC_Associativity, CT.Data);
    obj_CacheUpdate.initializeCache(IC_Sets, IC_Associativity, CT.Instruction);
    
    //reading arguments for mode and file
    mode=atoi(argv[1]);
    string fileName=argv[2];
    file.open(fileName);
    if(file.is_open())
    {
        int n;
        string input_Address;
        //reading file line by line
        while(file>>n)
        {
            //it reads n and adress in contigous fashion even on a single line
            //reading both n and address for n values other than 8 and 9 on a line, else only n is read
            //handling invalid address
            if(n==0||n==1||n==2||n==3||n==4)
            {
                file>>input_Address;
                if(input_Address.size()>address_Length/4)
                {
                    cout<<"Input address "<<input_Address<<" has exceeded the permitted limit"<<endl<<endl;
                    continue;
                }
                if(regex_match(input_Address,reg))
                {
                    cout<<input_Address<<" is an invalid address"<<endl<<endl;
                    continue;
                }
            }
            
            //Address bifurcation parameters are stored seperately for both Data and Instruction Caches as it has different cache specifications
            if (n == 2)
                obj_Address_Bifurcation.bifurcateAddress(input_Address,ICP.index_Size, address_Length, ICP.tag_Size, ICP.byte_Select);
            else
                obj_Address_Bifurcation.bifurcateAddress(input_Address,DCP.index_Size, address_Length, DCP.tag_Size, DCP.byte_Select);
            
            //case logic for different memory references
            switch(n)
            {
                    //read data request to L1 data cache
                case 0 :
                {
                    stats.Data_Read_Count++;
                    status = obj_CacheSetStatus.hit(obj_Address_Bifurcation.int_Index, DC_Associativity, obj_Address_Bifurcation.tag_Bits, CT.Data);
                    way = obj_CacheSetStatus.way;
                    if (!status)
                    {
                        stats.Data_Miss_Count++;
                        if (!obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data))
                        {
                            obj_CacheSetStatus.highestLRU(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data);
                            way = obj_CacheSetStatus.way;
                            if (obj_StateTransitions.checkState("M", CT.Data, obj_Address_Bifurcation.int_Index, way))
                            {
                                if (mode == 1)
                                {
                                    cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_CacheSetStatus.tag, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                    cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                }
                            }
                            else
                            {
                                if(mode == 1)
                                    cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                            }
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data, way);
                        }
                        else
                        {
                            obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data);
                            way = obj_CacheSetStatus.way;
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data, way);
                            if (mode == 1)
                                cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                        }
                        obj_StateTransitions.updateMESI("S", CT.Data, obj_Address_Bifurcation.int_Index, way);
                    }
                    else
                    {
                        stats.Data_Hit_Count++;
                        obj_LRU_Bits_Update.LRU_Update_Hit(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data, way);
                    }
                    obj_CacheUpdate.updateTag(way, obj_Address_Bifurcation.tag_Bits, CT.Data, obj_Address_Bifurcation.int_Index, DCP.tag_Size);
                    stats.Data_Hit_Ratio = (float)stats.Data_Hit_Count / (stats.Data_Hit_Count + stats.Data_Miss_Count);
                }
                    break;
                    //write data request to L1 data cache
                case 1 :
                {
                    stats.Data_Write_Count++;
                    status = obj_CacheSetStatus.hit(obj_Address_Bifurcation.int_Index, DC_Associativity, obj_Address_Bifurcation.tag_Bits, CT.Data);
                    way = obj_CacheSetStatus.way;
                    if (status)
                    {
                        obj_LRU_Bits_Update.LRU_Update_Hit(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data,way);
                        stats.Data_Hit_Count++;
                        if (obj_StateTransitions.checkState("S", CT.Data, obj_Address_Bifurcation.int_Index, way))
                        {
                            obj_StateTransitions.updateMESI("E", CT.Data, obj_Address_Bifurcation.int_Index, way);
                            if (mode == 1)
                                cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                        }
                        else
                            obj_StateTransitions.updateMESI("M", CT.Data, obj_Address_Bifurcation.int_Index, way);
                    }
                    else
                    {
                        stats.Data_Miss_Count++;
                        if (obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data))
                        {
                            obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data);
                            way = obj_CacheSetStatus.way;
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data, way);
                            if (mode == 1)
                            {
                                cout<<obj_DisplayMessages.GetMessage(4, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                            }
                        }
                        else
                        {
                            obj_CacheSetStatus.highestLRU(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data);
                            way = obj_CacheSetStatus.way;
                            if (!obj_StateTransitions.checkState("M", CT.Data, obj_Address_Bifurcation.int_Index, way))
                            {
                                if (mode == 1)
                                {
                                    cout<<obj_DisplayMessages.GetMessage(4, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                    cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                }
                            }
                            else
                            {
                                if(mode == 1)
                                {
                                    cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_CacheSetStatus.tag, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                    cout<<obj_DisplayMessages.GetMessage(4, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                    cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                                }
                            }
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, DC_Associativity, CT.Data, way);
                        }
                        obj_StateTransitions.updateMESI("E", CT.Data, obj_Address_Bifurcation.int_Index, way);
                    }
                    obj_CacheUpdate.updateTag(way, obj_Address_Bifurcation.tag_Bits, CT.Data, obj_Address_Bifurcation.int_Index, DCP.tag_Size);
                    stats.Data_Hit_Ratio = (float)stats.Data_Hit_Count / (stats.Data_Hit_Count + stats.Data_Miss_Count);
                }
                    break;
                    //instruction fetch (a read request to L1 instruction cache)
                case 2:
                {
                    stats.Instruction_Read_Count++;
                    status = obj_CacheSetStatus.hit(obj_Address_Bifurcation.int_Index, IC_Associativity, obj_Address_Bifurcation.tag_Bits, CT.Instruction);
                    way = obj_CacheSetStatus.way;
                    if (!status)
                    {
                        stats.Instruction_Miss_Count++;
                        if (!obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction))
                        {
                            obj_CacheSetStatus.highestLRU(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction);
                            way = obj_CacheSetStatus.way;
                            //Handling Modified state for few exception processor architectures where even write is done
                            //This is not required for this design, as Instruction cache is read only
                            if (obj_StateTransitions.checkState("M", CT.Instruction, obj_Address_Bifurcation.int_Index, way))
                            {
                                if (mode == 1)
                                {
                                    cout<<obj_DisplayMessages.GetMessage(2, obj_GetAddress.lineAddress(obj_CacheSetStatus.tag, obj_Address_Bifurcation.int_Index, ICP.byte_Select, ICP.index_Size))<<endl;
                                    cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, ICP.byte_Select, ICP.index_Size))<<endl;
                                }
                            }
                            else
                            {
                                if(mode == 1)
                                    cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, ICP.byte_Select, ICP.index_Size))<<endl;
                            }
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction, way);
                        }
                        else
                        {
                            obj_CacheSetStatus.invalidLine(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction);
                            way = obj_CacheSetStatus.way;
                            obj_LRU_Bits_Update.LRU_Update_Miss(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction, way);
                            if (mode == 1)
                                cout<<obj_DisplayMessages.GetMessage(3, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, ICP.byte_Select, ICP.index_Size))<<endl;
                        }
                        obj_StateTransitions.updateMESI("S", CT.Instruction, obj_Address_Bifurcation.int_Index, way);
                    }
                    else
                    {
                        stats.Instruction_Hit_Count++;
                        obj_LRU_Bits_Update.LRU_Update_Hit(obj_Address_Bifurcation.int_Index, IC_Associativity, CT.Instruction, way);
                    }
                    obj_CacheUpdate.updateTag(way, obj_Address_Bifurcation.tag_Bits, CT.Instruction, obj_Address_Bifurcation.int_Index, ICP.tag_Size);
                    stats.Instruction_Hit_Ratio = (float)stats.Instruction_Hit_Count / (stats.Instruction_Hit_Count + stats.Instruction_Miss_Count);
                }
                    break;
                    //invalidate command from L2
                case 3 :
                {
                    status = obj_CacheSetStatus.hit(obj_Address_Bifurcation.int_Index, DC_Associativity, obj_Address_Bifurcation.tag_Bits, CT.Data);
                    way = obj_CacheSetStatus.way;
                    if (status)
                    {
                        if (obj_StateTransitions.checkState("M", CT.Data, obj_Address_Bifurcation.int_Index, way))
                            cout<<"Modified Line shouldn't be invalidated -> Send \"Return Data to L2\" before invalidating..."<<endl<<endl;
                        else
                        {
                            obj_LRU_Bits_Update.LRU_Update_Invalidate(obj_Address_Bifurcation.int_Index, DC_Associativity, way);
                            obj_StateTransitions.updateMESI("I", CT.Data, obj_Address_Bifurcation.int_Index, way);
                        }
                    }
                    else
                        cout<< "Cache Line not in L1 Data Cache with address "<<obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size)<<endl<<endl;
                }
                    break;
                    //data request from L2 (in response to snoop)
                case 4 :
                {
                    status = obj_CacheSetStatus.hit(obj_Address_Bifurcation.int_Index, DC_Associativity, obj_Address_Bifurcation.tag_Bits, CT.Data);
                    way = obj_CacheSetStatus.way;
                    if (status)
                    {
                        if (obj_StateTransitions.checkState("M", CT.Data, obj_Address_Bifurcation.int_Index, way) && (mode == 1))
                            cout<<obj_DisplayMessages.GetMessage(1, obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size))<<endl;
                        obj_StateTransitions.updateMESI("S", CT.Data, obj_Address_Bifurcation.int_Index, way);
                    }
                    else
                        cout<< "Cache Line not in L1 Data Cache with address "<<obj_GetAddress.lineAddress(obj_Address_Bifurcation.tag_Bits, obj_Address_Bifurcation.int_Index, DCP.byte_Select, DCP.index_Size)<<endl<<endl;
                }
                    break;
                    //clear the cache and reset all state (and statistics)
                case 8:
                {
                    obj_CacheUpdate.initializeCache(DC_Sets, DC_Associativity, CT.Data);
                    obj_CacheUpdate.initializeCache(IC_Sets, IC_Associativity, CT.Instruction);
                    stats=obj_CacheUpdate.clearStatistics();
                    
                }
                    break;
                    //print contents and state of the cache (allow subsequent trace activity)
                case 9:
                {
                    cout<<"Data Cache Contents"<<endl;
                    obj_Display_Contents.Display_Cache_Contents(DC_Sets, DC_Associativity, CT.Data);
                    cout<<endl<<"Instruction Cache Contents"<<endl;
                    obj_Display_Contents.Display_Cache_Contents(IC_Sets, IC_Associativity, CT.Instruction);
                    cout<<endl<<endl;
                }
                    break;
                    
                default: cout<<"Invalid Memory Reference with n = "<<n<<endl<<endl;
                    break;
            }
        }
        file.close();
        //statistics generation --> divide by zero is handled
        //Display statistics
        if (stats.Data_Hit_Count == 0 && stats.Data_Miss_Count == 0)
            stats.Data_Hit_Ratio = 0;
        else
            stats.Data_Hit_Ratio = (float)stats.Data_Hit_Count / (stats.Data_Hit_Count + stats.Data_Miss_Count);
        if (stats.Instruction_Hit_Count == 0 && stats.Instruction_Miss_Count == 0)
            stats.Instruction_Hit_Ratio = 0;
        else
            stats.Instruction_Hit_Ratio = (float)stats.Instruction_Hit_Count / (stats.Instruction_Hit_Count + stats.Instruction_Miss_Count);
        obj_Display_Contents.Display_Statistics(stats.Data_Read_Count,stats.Data_Write_Count, stats.Data_Hit_Count, stats.Data_Miss_Count, stats.Data_Hit_Ratio, stats.Instruction_Read_Count, stats.Instruction_Hit_Count, stats.Instruction_Miss_Count, stats.Instruction_Hit_Ratio);
    }
    else
    {
        cout<<"File not opened";
    }
    
    return 0;
}


