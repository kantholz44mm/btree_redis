#include "btree2020.cpp"

int main()
{
    DataStructureWrapper btree(true);
    
    // Insert (Key must be unique)
    btree.insert((uint8_t*)"key", 3, (uint8_t*)"val", 3);
    
    // Lookup
    unsigned sz;
    uint8_t* res = btree.lookup((uint8_t*)"key", 3, sz);
    
    // Range Scan (return false to stop)
    uint8_t buf[BTreeNode::maxKVSize];
    btree.range_lookup((uint8_t*)"k", 1, buf, [](unsigned kL, uint8_t* p, unsigned pL) {
        return true; 
    });
    
    // Remove
    btree.remove((uint8_t*)"key", 3);
}
