#include <iostream>
#include <bitset>
#include <string>
#include <unordered_map>
#include <cmath>
#include <list>
#include <sstream>
#include <fstream>
#include <ctime>

using namespace std;

/**
 * @brief A file system which allocates contigous blocks of memory to each file.
 * 
 * @tparam N size of the memory
 */
template<size_t N = 1024>
class ContiguousFileSystem{
    public:

    enum Strategy {FIRST_FIT, BEST_FIT, NEXT_FIT, WORST_FIT};

    class File {
        int filesize;
        int start_block;
        File(int filesize, int start) : filesize(filesize), start_block(start) {}
        friend class ContiguousFileSystem;
    };

    /**
     * @brief Construct a new Contiguous File System object
     * 
     * @param alg strategy for finding a hole. FIRST_FIT/BEST_FIT/WORST_FIT/NEXT_FIT. default = FIRST_FIT.
     */
    ContiguousFileSystem(Strategy alg = ContiguousFileSystem<N> :: FIRST_FIT) : memory_size(N), 
                                                     used_memory(0),
                                                     start_index(0), 
                                                     strategy(alg) {}
    
    /**
     * @brief Creates a file of given size. Allocates contiguous blocks of memory.
     * 
     * @param filename name of the file.
     * @param filesize size of the file.
     */
    void create(string filename, int filesize) {
        if(file_map.find(filename) != file_map.end()){
            cerr << "ContigousFileSystem::create() : Filename " << filename << " already taken\n";
            return;
        }

        // get the starting block of the alloted chunk.
        int start = get_index(filesize);
        
        // if the required size can't be alloted contiguously return.
        if(start == -1){
            cerr << "ContiguousFileSystem::create() : Cannot allocate " << filesize << " blocks for " << filename << endl;
            return;
        }

        cout << "ContiguousFileSystem::create() : File " << filename << " created with starting block - " << start << endl;

        // mark the blocks as used.
        for(int i = 0; i < filesize; i++){
            memory_map.set(i+start);
        }

        File* fp = new File(filesize, start);
        used_memory += filesize;
        file_map[filename] = fp;
        return;
    }

    /**
     * @brief reads 'size' blocks of the file from given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to read.
     * @param offset block number to start reading.
     * @return int - number of block accesses taken to read the file.
     */
    int read(string filename, int size=-1, int offset = 0) {
        int block_access = 1;

        //if file is not present in the filesytem, return.
        if(file_map.find(filename) == file_map.end()){
            cerr << "ContiguousFileSystem:read() : File " << filename << " not found\n";
            return block_access;
        }
        
        File* fp = file_map[filename];

        size = (size==-1) ? fp->filesize : size;

        // read blocks from the offset.
        int read = 0;
        for(int i = 0; i < fp->filesize && read < size; i++){
            if (i >= offset){
                cout << "ContiguousFileSystem:read() : Reading block " << fp->start_block + i << endl;
                read++;
            }
            block_access++;
        }

        cout << "ContiguousFileSystem:read() : Total blocks read : " << read << endl;

        return size+1;
    }

    /**
     * @brief removes the given file from the filesystem.
     * 
     * @param filename name of the file.
     */
    void delete_file(string filename){
        //if file is not present in the filesytem, return.
        if(file_map.find(filename) == file_map.end()){
            cerr << "ContigousFileSystem::delete(): File not found.\n";
            return;
        }

        ContiguousFileSystem::File* fp = file_map[filename];

        // mark the blocks as available.
        cout << "ContiguousFileSystem::delete_file() : deallocating blocks\n";
        for(int i = 0; i < fp->filesize; i++){
            memory_map.reset(fp->start_block + i);
        }

        // delete metafile.
        used_memory -= fp->filesize;
        delete fp;
        file_map.erase(filename);
        cout << "ContiguousFileSystem::delete_file() : " <<  filename << " deleted\n";
    }

    /**
     * @brief writes 'size' blocks starting from the given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to write.
     * @param offset starting block to write from.
     * @return int - number of block accesses taken to write the file
     */
    int write(string filename, int size, int offset = 0){
        int block_access = 1;
        //if file is not present in the filesytem, return.
        if(file_map.find(filename) == file_map.end()){
            cerr << "ContigousFileSystem::write() : File " << filename << " not found\n";
            return block_access;
        }

        ContiguousFileSystem::File* fp = file_map[filename];

        if(size + offset > fp->filesize){
            // in case of overflow, check if contiguous blocks can be allotted.
            int req = size + offset - fp->filesize;
            int available;
            int end = fp->start_block + fp->filesize;

            for(available = 0; available < req; available++){
                if(memory_map[available+end] == 1)
                    break;
            }

            if(available != req){
                cerr << "ContiguousFileSystem::write() : Blocks for given size cannot be allocated.\n";
                return block_access;
            }

            //if it is possible to allot contiguous blocks, mark them as used.
            for(available = 0; available < req; available++)
                memory_map.set(available+end);
            

            fp->filesize += req; 

            used_memory += req;
        }

        int start = fp->start_block + offset;
        for(int i = 0; i < size; i++){
            block_access++;
            cout << "ContigousFileSystem::write() : Writing block " <<  start + i << endl; 
        }

        cout << "ContigousFileSystem::write() : Total blocks written " << size << endl;
        return size+1;
    }

    /**
     * @brief Get the storage efficiency of the file system.
     * 
     * @return float - returns the ratio of used memory to the total memory size.
     */
    float get_storage_efficiency(){
        //cout << "used memory : " << used_memory << endl;
        return (float)used_memory/N;
    }

    protected:

    bitset<N> memory_map;
    unordered_map<string, File*> file_map;
    const int memory_size;
    int start_index;
    Strategy strategy;
    int used_memory;

    /**
     * @brief finds the first hole that can accomodate the given size.
     * 
     * @param size size of the file to fit.
     * @return int - block index of the first hole.
     */
    int first_fit(int size){
        int i = 0;
        while(i < memory_size){
            if(memory_map[i] == 0){
                int j = i;
                //find the size of the hole.
                while(j < memory_size && j < i+size && memory_map[j] == 0)
                    j++;

                //if the current hole can fit the size, return.
                if(j == i+size)
                    return i;
                
                i += j;
            }
            else    
                i++;
        }
        return -1;
    }

    /**
     * @brief finds the smallest hole that can fit the given size.
     * 
     * @param size size of the file to fit.
     * @return int - block index of the smallest hole.
     */
    int best_fit(int size){
        int i = 0;
        int min_size = INT_MAX;
        int ind = -1;
        
        while(i < memory_size){
            if(memory_map[i] == 0){
                int j = i;
                //find the size of the hole.
                while(j < memory_size && memory_map[j] == 0)
                    j++;
                
                // if the current hole is smaller, update ind.
                if(j >= i+size && j-i <= min_size){
                    min_size = j-i;
                    ind = i;
                }

                i = j;
            }
            else    
                i++;
        }

        return ind;
    }

    /**
     * @brief finds the largest hole that can fit the given size.
     * 
     * @param size size of the file to fit.
     * @return int - block index of the largest hole.
     */
    int worst_fit(int size){
        int i = 0;
        int max_size = 0;
        int ind = -1;

        while(i < memory_size){
            if(memory_map[i] == 0){
                int j = i;
                //find the size of the hole
                while(j < memory_size && memory_map[j] == 0)
                    j++;
                
                // if the current hole is larger, update ind.
                if(j >= i+size && j-i > max_size){
                    max_size = j-i;
                    ind = i;
                }

                i += j;
            }
            else    
                i++;
        }
        return ind;
    }

    /**
     * @brief finds the first hole that can fit the size starting from the point where the last search ended.
     * 
     * @param size size of the file to fit.
     * @return int - block index of the hole.
     */
    int next_fit(int size){
        int i = start_index;
        int count = 0;
        while(count < memory_size){
            if(memory_map[i] == 0){
                int j = i;
                while(j < memory_size && j < i+size && memory_map[j] == 0)
                    j++;

                if(j == i+size){
                    start_index = (j+i)%memory_size;
                    return i;
                }
                
                i = (i + j)%memory_size;
                count += j;
            }
            else{    
                i = (i+1)%memory_size;
                count++;
            }

        }
        return -1;
    }

    /**
     * @brief Get the starting block index of the alloted contiguous block based on the strategy.
     * 
     * @param size size to fit.
     * @return int starting index of the block.
     */
    int get_index(int size){
        if(strategy == FIRST_FIT)
            return first_fit(size);
        else if(strategy == BEST_FIT)
            return best_fit(size);
        else if(strategy == WORST_FIT)
            return worst_fit(size);
        else
            return next_fit(size);
    }

};

/**
 * @brief A file system which allocates linked list of blocks to each file.
 * 
 * @tparam N size of the memory.
 */
template<size_t N = 1024>
class LinkedFileSystem{

    private:

    struct block{
        int id;
        block* next;
        block(int id, block* next = nullptr){
            this->id = id;
            this->next = next;
        }
    };

    class File {
        int filesize;
        block* start_block;
        File(int filesize, block* start) : filesize(filesize), start_block(start) {}
        friend class LinkedFileSystem;
    };

    list<block*> free_list;
    unordered_map<string, File*> file_map;
    int used_memory;

    public:

    /**
     * @brief Construct a new Linked File System object
     * 
     */
    LinkedFileSystem() : used_memory(0){
        for(int i = 0; i < N; i++){
            block* b = new block(i);
            free_list.push_back(b);
        }
    }

    /**
     * @brief Creates a file of given size. Allocates non contiguous linked blocks of memory.
     * 
     * @param filename name of the file.
     * @param filesize size of the file.
     */
    void create(string filename, int size){
        if(file_map.find(filename) != file_map.end()){
            cerr << "LinkedFileSystem::create() : Filename already taken\n";
            return;
        }

        //if there are less number of blocks in free list than required, return.
        if(free_list.size() < size){
            cerr << "LinkedFileSystem::write() : cannot allocate " << size << " blocks for " << filename << endl;
            return;
        }

        block* start = free_list.front();
        free_list.pop_front();

        File* fp = new File(size, start);
        cout << "LinkedFileSystem::write() : File " << filename << " created with starting block " << start->id << endl;

        // remove from free list and link it to the file.
        for(int i = 1; i < size; i++){
            start->next = free_list.front();
            free_list.pop_front();
            start = start->next;
        }

        used_memory += size;

        file_map[filename] = fp;
    }

    /**
     * @brief reads 'size' blocks of the file from given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to read.
     * @param offset block number to start reading.
     * @return int - number of block accesses taken to read the file.
     */
    int read(string filename, int size, int offset = 0){
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "LinkedFileSystem::read() : File " << filename << " not found\n";
            return block_access;
        }

        File* fp = file_map[filename];
        block* start = fp->start_block;

        int bno = 0, read = 0;

        //iterate through the list and read the blocks greater than offset.
        while(start != nullptr && read < size){
            if(bno >= offset){
                read++;
                cout << "LinkedFileSystem::read() : Reading block " << start->id << endl;
            }
            bno++;
            start = start->next;
            block_access++;
        }

        cout << "LinkedFileSystem::read() : Total blocks read : " << read << endl;

        return block_access;
    }

    /**
     * @brief writes 'size' blocks starting from the given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to write.
     * @param offset starting block to write from.
     * @return int - number of block accesses taken to write the file
     */
    int write(string filename, int size, int offset = 0){
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "LinkedFileSystem::write() : File " << filename << " not found\n";
            return block_access;
        }
        
        File* fp = file_map[filename];

        // if the required number of overflow blocks is greater than the size of the free list, return.
        if(offset + size > fp->filesize && offset + size - fp->filesize  > free_list.size()){
            cerr << "LinkedFileSystem::write() : cannot allocate memory for write\n";
            return block_access;
        }

        block *curr = fp->start_block, *prev = nullptr;
        
        int bno = 0, written = 0;

        //iterate through the list overwrite if block number greater than offset. allocate overflow blocks as when required.
        while(written != size){
            if(!curr){
                curr = prev->next = free_list.front();
                free_list.pop_front();
                cout << "LinkedFileSystem::write() : Allocating new block " << curr->id << endl; 
                fp->filesize++;
                used_memory++;
            }

            if(bno >= offset){
                cout << "LinkedFileSystem::write() : writing block " << curr->id << endl;
                written++;
            }

            bno++;
            prev = curr;
            curr = curr->next;
            block_access++;
        }

        clock_t end_time = clock();

        cout << "LinkedFileSystem::write() : Total blocks written : " << written << endl;

        return block_access;
    }

    /**
     * @brief removes the given file from the filesystem.
     * 
     * @param filename name of the file.
     */
    void delete_file(string filename){
        if(file_map.find(filename) == file_map.end()){
            cerr << "LinkedFileSystem::delete() : File not found.\n";
            return;
        }

        File *fp = file_map[filename];
        block *curr = fp->start_block;

        cout << "LinkedFileSystem::delete() : deallocating file blocks\n";

        //delink the blocks from the file and add it to free list.
        while(curr){
            block* next = curr->next;
            curr->next = nullptr;
            free_list.push_back(curr);
            curr = next;
        }

        used_memory -= fp->filesize;
        file_map.erase(filename);
        delete fp;

        cout << "LinkedFileSystem::delete() : " << filename << " deleted\n";
    }

    /**
     * @brief Get the storage efficiency of the file system.
     * 
     * @return float - returns the ratio of used memory to the total memory size.
     */
    float get_storage_efficiency(){
        return (float)used_memory/N;
    }
};

/**
 * @brief A file system which allocates non contiguous blocks to each file. the block numbers are stored in the metafile.
 * 
 * @tparam N size of the memory.
 */
template<size_t N = 1024>
class IndexedFileSystem{
    private:
    class File {
        int filesize;
        vector<int> block_indices;
        File(int filesize) : filesize(filesize) {}
        friend class IndexedFileSystem;
    };

    unordered_map<string, File*> file_map;
    list<int> free_list;
    int used_memory;

    public:
    /**
     * @brief Construct a new Indexed File System object
     * 
     */
    IndexedFileSystem() : used_memory(0){
        for(int i = 0; i < N; i++)
            free_list.push_back(i);
    }

    /**
     * @brief Creates a file of given size. Allocates non contiguous blocks of memory.
     * 
     * @param filename name of the file.
     * @param filesize size of the file.
     */
    void create(string filename, int size){
        if(file_map.find(filename) != file_map.end()){
            cerr << "IndexedFileSystem::create() : Filename already taken\n";
            return;
        }

        // if more blocks are required than it is available, return.
        if(free_list.size() < size){
            cerr << "IndexedFileSystem::create() : cannot allocate " << size << " blocks for " << filename << "\n";
            return;
        }

        File* fp = new File(size);

        //remove blocks from free list and add it to the list of indices.
        for(int i = 0; i < size; i++){
            fp->block_indices.push_back(free_list.front());
            free_list.pop_front();
        }

        used_memory += size;
        file_map[filename] = fp;
        cout << "IndexedFileSystem::create() : file " << filename << " created starting block " << fp->block_indices[0] << endl;
    }

    /**
     * @brief reads 'size' blocks of the file from given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to read.
     * @param offset block number to start reading.
     * @return int - number of block accesses taken to read the file.
     */
    int read(string filename, int size = -1, int offset = 0){
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "IndexedAllocation::read() : File " << filename << " not found\n";
            return block_access;
        }

        File* fp = file_map[filename];

        size = (size == -1) ? fp->filesize : size;
        int read = 0;

        // iterate through the list of indices from the offset.
        for(int i = offset; i < fp->filesize && read < size; i++){
            cout << "IndexedAllocation::read() : Reading block " << fp->block_indices[i] << endl;
            read++;
            block_access++;
        }

        cout << "IndexedAllocation::read() : Total blocks read : " << read << endl;

        return block_access;
    }

    /**
     * @brief writes 'size' blocks starting from the given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to write.
     * @param offset starting block to write from.
     * @return int - number of block accesses taken to write the file
     */
    int write(string filename, int size, int offset = 0){
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "IndexedFileSystem::write() : File " << filename << " not found\n";
            return block_access;
        }

        File* fp = file_map[filename];
        int written = 0;
        int bno = offset;

        // if more overflow blocks are required than it is available, return.
        if(offset + size > fp->filesize && offset + size - fp->filesize  > free_list.size()){
            cerr << "IndexedFileSystem::write() : cannot allocate memory for write\n";
            return block_access;
        }

        // allocate new blocks from free list if required.
        while(written < size){
            if(bno >= fp->block_indices.size()){
                fp->block_indices.push_back(free_list.front());
                cout << "IndexedFileSystem::write() : Allocating new block " << free_list.front() << endl;
                free_list.pop_front();
                fp->filesize++;
                used_memory++;
            }
            
            cout << "IndexedFileSystem::write() : Writing block " << fp->block_indices[bno] << endl;
            block_access++;
            written++;
            bno++;
        }

        cout << "IndexedFileSystem::write() : Total blocks written : " << written << endl;

        return block_access;
    }

    /**
     * @brief removes the given file from the filesystem.
     * 
     * @param filename name of the file.
     */
    void delete_file(string filename){
        if(file_map.find(filename) == file_map.end()){
            cerr << "IndexedFleSystem::delete() : File not found.\n";
            return;
        }
        File *fp = file_map[filename];

        // remove the block index from index table and add it to free list.
        cout << "IndexedFleSystem::delete() : deallocating file blocks\n";
        for(int blocks : fp->block_indices)
            free_list.push_back(blocks);
        
        used_memory -= fp->filesize;
        delete fp;
        file_map.erase(filename);
        cout << "IndexedFleSystem::delete() : File " << filename << " deleted\n";
    }

    /**
     * @brief Get the storage efficiency of the file system.
     * 
     * @return float - returns the ratio of used memory to the total memory size.
     */
    float get_storage_efficiency(){
        return (float)used_memory/N;
    }

};

/**
 * @brief A file system that allocates initial contiguous block. contiguous overflow blocks are linked. Inherits from ContiguousFileSystem
 * 
 * @tparam N size of the memory
 */
template<size_t N=1024>
class ModifiedContiguousFileSystem : public ContiguousFileSystem<N>{
    private:
    struct block{
        int start_block;
        int size;
        block* next;
        block(int strt, int sz, block* nxt = nullptr){
            start_block = strt;
            size = sz;
            next = nxt;
        }
    };
    class File{
        int filesize;
        block* start;
        File(int filesize, block* start) : filesize(filesize), start(start) {}
        friend class ModifiedContiguousFileSystem;
    };

    unordered_map<string, File*> file_map;

    public: 
    ModifiedContiguousFileSystem(typename ContiguousFileSystem<N>::Strategy alg = ContiguousFileSystem<N> :: FIRST_FIT) : ContiguousFileSystem<N>(alg){}

    /**
     * @brief Creates a file of given size. Allocates contiguous linked blocks of memory.
     * 
     * @param filename name of the file.
     * @param filesize size of the file.
     */
    void create(string filename, int filesize) {
        if(file_map.find(filename) != file_map.end()){
            cerr << "ModifiedContiguousFileSystem::create() : Filename already taken\n";
            return;
        }

        // get the start index of allotted chunk.
        int start = this->get_index(filesize);

        // if it is not possible to allot contiguous chunk return.
        if(start == -1){
            cerr << "ModifiedContiguousFileSystem::create() : Can't allocate " << filesize << " blocks for file " << filename << "\n";
            return;
        }

        cout << "ModifiedContiguousFileSystem::create() : File created starting block - " << start << endl;

        // mark the allotted blocks as used.
        for(int i = 0; i < filesize; i++){
            this->memory_map.set(i+start);
        }

        this->used_memory += filesize;
        File* fp = new File(filesize, new block(start, filesize));
        file_map[filename] = fp;
        return;
    }

    /**
     * @brief writes 'size' blocks starting from the given offset. contiguous blocks are alloted and linked to the file.
     * 
     * @param filename name of the file.
     * @param size number of blocks to write.
     * @param offset starting block to write from.
     * @return int - number of block accesses taken to write the file
     */
    int write(string filename, int size, int offset) {
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "ModifiedContiguousFileSystem::write() : File " << filename << " not found\n";
            return 0;
        }

        File* fp = file_map[filename];
        block* newblock = nullptr;
        if(size + offset > fp->filesize){
            // check if a contiguous chunk can be allotted for the overflow data. 
            int req = size + offset - fp->filesize;
            int available;

            int index = this->get_index(req);

            // if possible, mark the blocks as used.
            for(int i = 0; i < req; i++){
                this->memory_map.set(i+index);
            }

            if(index == -1){
                cerr << "ModifiedContiguousFileSystem::write() : cannot allocate overflow blocks\n";
                return block_access;
            }
            
            newblock = new block(index, req);

            fp->filesize += req; 

            this->used_memory += req;
        }

        int bno = 0, written = 0;
        block* b = fp->start;
        block* last = nullptr;

        while(b && written < size){
            block_access++;
            if(offset >= bno && offset < bno + b->size){
                int wrt = offset-bno;
                if(wrt == 0)
                    block_access--;
                for(int i = wrt; i < b->size && written != size; i++){
                    cout << "ModifiedContiguousFileSystem::write() : Writing block " << b->start_block + i << endl;
                    block_access++;
                    written++;
                }
                offset += written;
            }
            bno += b->size;
            last = b;
            b = b->next;
        }

        if(newblock){
            cout << "ModifiedContiguousFileSystem::write() : Allocating overflow blocks\n" ;
            for(int i = 0; i < newblock->size; i++){
                cout << "ModifiedContiguousFileSystem::write() : Writing block " << newblock->start_block + i << endl;
                block_access++;
                written++;
            }
            last->next = newblock;
        }

        cout << "ModifiedContiguousFileSystem::write() : Total blocks written : " << written << endl;

        return size+1;
    }

    /**
     * @brief reads 'size' blocks of the file from given offset.
     * 
     * @param filename name of the file.
     * @param size number of blocks to read.
     * @param offset block number to start reading.
     * @return int - number of block accesses taken to read the file.
     */
    int read(string filename, int size=-1, int offset = 0) {
        int block_access = 1;

        if(file_map.find(filename) == file_map.end()){
            cerr << "ModifiedContigousFileSystem::read() : File " << filename << " not found\n";
            return block_access;
        }
        
        File* fp = file_map[filename];

        size = (size==-1) ? fp->filesize : size;

        int read = 0, bno = 0;
        block* b = fp->start;

        // find the block where the offset block is present and start reading.
        while(b && read < size){
            block_access++;
            if(offset >= bno && offset < bno + b->size){
                int rd = offset - bno;
                if(rd == 0)
                    block_access--;
                for(int i = rd; i < b->size && read < size; i++){
                    cout << "ModifiedContigousFileSystem::read() : Reading block " << b->start_block + i << endl;
                    block_access++;
                    read++;
                }
                offset += read;
            }
            bno += b->size;
            b = b->next;
        }

        cout << "ModifiedContigousFileSystem::read() : Total blocks read : " << read << endl;

        return size+1;
    }

    /**
     * @brief removes the given file from the filesystem.
     * 
     * @param filename name of the file.
     */
    void delete_file(string filename) {
        if(file_map.find(filename) == file_map.end()){
            cerr << "ModifiedContigousFileSystem::delete(): File not found.\n";
            return;
        }

        cout << "ModifiedContigousFileSystem::delete() : deallocating block\n";
        File* fp = file_map[filename];
        block* b = fp->start;

        //iterate through each chunk, marking each block in the chunk as free.
        while(b){
            for(int i = 0; i < b->size; i++){
                this->memory_map.reset(b->start_block+i);
            }
            block* next = b->next;
            delete b;
            b = next;
        }
        
        this->used_memory -= fp->filesize;
        file_map.erase(filename);
        delete fp;

        cout << "ModifiedContigousFileSystem::delete() : " << filename << " deleted\n";
    }
};

int main(int argc, char* argv[]){
    if(argc < 2){
        cerr << "Provide input query file\n";
        exit(EXIT_FAILURE);
    }

    ModifiedContiguousFileSystem<500> mcfs;
    LinkedFileSystem<500> lfs;
    IndexedFileSystem<500> ifs;
    ContiguousFileSystem<500> cfs(ContiguousFileSystem<500>::BEST_FIT);

    ifstream infile(argv[1]);
    string line;

    int cfs_time = 0, lfs_time = 0, mcfs_time = 0, ifs_time = 0;
    int success_cfs = 0, success_lfs = 0, success_mcfs = 0, success_ifs = 0;

    while (getline(infile, line)) {
        stringstream ss(line);
        string operation, file_name;
        int file_size, size, offset;

        ss >> operation >> file_name;

        if (operation == "CREATE") {
            ss >> file_size;
            mcfs.create(file_name, file_size);
            cfs.create(file_name, file_size);
            ifs.create(file_name, file_size);
            lfs.create(file_name, file_size);
        } 
        else if (operation == "READ") {
            ss >> size >> offset;
            int ba = 0;
            ba = mcfs.read(file_name, size, offset);
            if(ba != 1){
                success_mcfs++;
                mcfs_time += ba;
            }
            ba = cfs.read(file_name, size, offset);
            if(ba != 1){
                success_cfs++;
                cfs_time += ba;
            }
            ba = ifs.read(file_name, size, offset);
            if(ba != 1){
                success_ifs++;
                ifs_time += ba;
            }
            ba = lfs.read(file_name, size, offset);
            if(ba != 1){
                success_lfs++;
                lfs_time += ba;
            }
        } 
        else if (operation == "WRITE") {
            ss >> size >> offset;
            int ba;
            ba = mcfs.write(file_name, size, offset);
            if(ba != 1){
                success_mcfs++;
                mcfs_time += ba;
            }
            ba = cfs.write(file_name, size, offset);
            if(ba != 1){
                success_cfs++;
                cfs_time += ba;
            }
            ba = ifs.write(file_name, size, offset);
            if(ba != 1){
                success_ifs++;
                ifs_time += ba;
            }
            ba = lfs.write(file_name, size, offset);
            if(ba != 1){
                success_lfs++;
                lfs_time += ba;
            }
        } 
        else if (operation == "DELETE_FILE") {
            mcfs.delete_file(file_name);
            cfs.delete_file(file_name);
            ifs.delete_file(file_name);
            lfs.delete_file(file_name);
        } 
        else {
            cout << "Invalid operation: " << operation << "\n";
        }
    }

    cout << "\nStorage efficiency\n";
    cout << "cfs: " << cfs.get_storage_efficiency() << endl;
    cout << "lfs: " << lfs.get_storage_efficiency() << endl;
    cout << "ifs: " << ifs.get_storage_efficiency() << endl;
    cout << "mcfs: " << mcfs.get_storage_efficiency() << endl;

    cout << "\nAverage block accesses for read/write queries\n";
    cout << "cfs " << (float)cfs_time /success_cfs  << endl;
    cout << "lfs " << (float)lfs_time /success_lfs << endl;
    cout << "ifs "  << (float)ifs_time /success_ifs << endl;
    cout << "mcfs " << (float)mcfs_time /success_mcfs << endl;
}

