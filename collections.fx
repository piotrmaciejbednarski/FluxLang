// ================================================================
// FLUX COLLECTIONS LIBRARY
// collections.fx - Generic data structures using void* storage
// ================================================================

import "types.fx";
import "system.fx";

namespace standard
{
    namespace collections
    {
        using standard::types::basic;
        using standard::system;
        
        // ================================================================
        // GENERIC DYNAMIC ARRAY
        // ================================================================
        
        object array
        {
            void* adata;
            basic::i32 length;
            basic::i32 capacity;
            basic::ui32 element_size;
            
            def __init(basic::ui32 elem_size) -> this
            {
                this.adata = void;
                this.length = 0;
                this.capacity = 0;
                this.element_size = elem_size;
                return this;
            };
            
            def __exit() -> void
            {
                if (this.adata is !void)
                {
                    api::deallocate_memory(this.adata, (basic::ui64)(this.capacity * this.element_size));
                };
                return void;
            };
            
            def size() -> basic::i32
            {
                return this.length;
            };
            
            def is_empty() -> bool
            {
                return this.length == 0;
            };
            
            def reserve(basic::i32 new_capacity) -> bool
            {
                if (new_capacity <= this.capacity) { return true; };
                
                void* new_data = api::allocate_memory((basic::ui64)(new_capacity * this.element_size));
                if (new_data is void) { return false; };
                
                if (this.adata is !void)
                {
                    // Copy existing data
                    basic::ui8* src = (basic::ui8*)this.adata;
                    basic::ui8* dst = (basic::ui8*)new_data;
                    basic::ui64 copy_size = (basic::ui64)(this.length * this.element_size);
                    
                    for (basic::ui64 i = 0; i < copy_size; i++)
                    {
                        dst[i] = src[i];
                    };
                    
                    api::deallocate_memory(this.adata, (basic::ui64)(this.capacity * this.element_size));
                };
                
                this.adata = new_data;
                this.capacity = new_capacity;
                return true;
            };
            
            def grow() -> bool
            {
                basic::i32 new_capacity = (this.capacity == 0) ? 8 : this.capacity * 2;
                return this.reserve(new_capacity);
            };
            
            def push(void* element) -> bool
            {
                if (this.length >= this.capacity)
                {
                    if (!this.grow()) { return false; };
                };
                
                // Copy element to array
                basic::ui8* src = (basic::ui8*)element;
                basic::ui8* dst = (basic::ui8*)this.adata + (this.length * this.element_size);
                
                for (basic::ui32 i = 0; i < this.element_size; i++)
                {
                    dst[i] = src[i];
                };
                
                this.length++;
                return true;
            };
            
            def pop(void* out_element) -> bool
            {
                if (this.length == 0) { return false; };
                
                this.length--;
                
                // Copy element out
                basic::ui8* src = (basic::ui8*)this.adata + (this.length * this.element_size);
                basic::ui8* dst = (basic::ui8*)out_element;
                
                for (basic::ui32 i = 0; i < this.element_size; i++)
                {
                    dst[i] = src[i];
                };
                
                return true;
            };
            
            def at(basic::i32 index) -> void*
            {
                if (index >= this.length or index < 0) { return void; };
                return (basic::ui8*)this.adata + (index * this.element_size);
            };
            
            def set(basic::i32 index, void* element) -> bool
            {
                if (index >= this.length or index < 0) { return false; };
                
                basic::ui8* src = (basic::ui8*)element;
                basic::ui8* dst = (basic::ui8*)this.adata + (index * this.element_size);
                
                for (basic::ui32 i = 0; i < this.element_size; i++)
                {
                    dst[i] = src[i];
                };
                
                return true;
            };
            
            def clear() -> void
            {
                this.length = 0;
                return void;
            };
        };
        
        // ================================================================
        // GENERIC HASH MAP
        // ================================================================
        
        object map
        {
            object entry
            {
                void* key;
                void* value;
                entry* next;
                bool is_occupied;
                
                def __init() -> this
                {
                    this.key = void;
                    this.value = void;
                    this.next = void;
                    this.is_occupied = false;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
            };
            
            entry* buckets;
            basic::i32 bucket_count;
            basic::i32 size;
            basic::ui32 key_size;
            basic::ui32 value_size;
            basic::ui32 (*hash_func)(void*);
            bool (*compare_func)(void*, void*);
            
            def __init(basic::ui32 k_size, basic::ui32 v_size, basic::ui32 (*hash_fn)(void*), bool (*cmp_fn)(void*, void*)) -> this
            {
                this.bucket_count = 16;
                this.size = 0;
                this.key_size = k_size;
                this.value_size = v_size;
                this.hash_func = hash_fn;
                this.compare_func = cmp_fn;
                
                this.buckets = (entry*)api::allocate_memory((basic::ui64)(this.bucket_count * sizeof(entry)));
                
                for (basic::i32 i = 0; i < this.bucket_count; i++)
                {
                    this.buckets[i] = entry();
                };
                
                return this;
            };
            
            def __exit() -> void
            {
                if (this.buckets is !void)
                {
                    for (basic::i32 i = 0; i < this.bucket_count; i++)
                    {
                        entry* current = this.buckets[i].next;
                        while (current is !void)
                        {
                            entry* next = current.next;
                            if (current.key is !void)
                            {
                                api::deallocate_memory(current.key, this.key_size);
                            };
                            if (current.value is !void)
                            {
                                api::deallocate_memory(current.value, this.value_size);
                            };
                            api::deallocate_memory(current, sizeof(entry));
                            current = next;
                        };
                    };
                    
                    api::deallocate_memory(this.buckets, (basic::ui64)(this.bucket_count * sizeof(entry)));
                };
                return void;
            };
            
            def get_bucket_index(void* key) -> basic::i32
            {
                basic::ui32 hash_value = (*this.hash_func)(key);
                return (basic::i32)(hash_value % (basic::ui32)this.bucket_count);
            };
            
            def put(void* key, void* value) -> bool
            {
                basic::i32 bucket_index = this.get_bucket_index(key);
                entry* bucket = &this.buckets[bucket_index];
                
                if (!bucket->is_occupied)
                {
                    bucket->key = api::allocate_memory(this.key_size);
                    bucket->value = api::allocate_memory(this.value_size);
                    if (bucket->key is void or bucket->value is void) { return false; };
                    
                    // Copy key and value
                    basic::ui8* key_src = (basic::ui8*)key;
                    basic::ui8* key_dst = (basic::ui8*)bucket->key;
                    basic::ui8* val_src = (basic::ui8*)value;
                    basic::ui8* val_dst = (basic::ui8*)bucket->value;
                    
                    for (basic::ui32 i = 0; i < this.key_size; i++)
                    {
                        key_dst[i] = key_src[i];
                    };
                    for (basic::ui32 i = 0; i < this.value_size; i++)
                    {
                        val_dst[i] = val_src[i];
                    };
                    
                    bucket->is_occupied = true;
                    this.size++;
                    return true;
                };
                
                // Check for existing key in chain
                entry* current = bucket;
                while (current is !void)
                {
                    if (current.is_occupied and (*this.compare_func)(current.key, key))
                    {
                        // Update existing value
                        basic::ui8* val_src = (basic::ui8*)value;
                        basic::ui8* val_dst = (basic::ui8*)current.value;
                        
                        for (basic::ui32 i = 0; i < this.value_size; i++)
                        {
                            val_dst[i] = val_src[i];
                        };
                        
                        return true;
                    };
                    
                    if (current.next is void) { break; };
                    current = current.next;
                };
                
                // Add new entry to chain
                entry* new_entry = (entry*)api::allocate_memory(sizeof(entry));
                if (new_entry is void) { return false; };
                
                *new_entry = entry();
                new_entry->key = api::allocate_memory(this.key_size);
                new_entry->value = api::allocate_memory(this.value_size);
                if (new_entry->key is void or new_entry->value is void)
                {
                    api::deallocate_memory(new_entry, sizeof(entry));
                    return false;
                };
                
                // Copy key and value
                basic::ui8* key_src = (basic::ui8*)key;
                basic::ui8* key_dst = (basic::ui8*)new_entry->key;
                basic::ui8* val_src = (basic::ui8*)value;
                basic::ui8* val_dst = (basic::ui8*)new_entry->value;
                
                for (basic::ui32 i = 0; i < this.key_size; i++)
                {
                    key_dst[i] = key_src[i];
                };
                for (basic::ui32 i = 0; i < this.value_size; i++)
                {
                    val_dst[i] = val_src[i];
                };
                
                new_entry.is_occupied = true;
                current.next = new_entry;
                this.size++;
                return true;
            };
            
            def get(void* key) -> void*
            {
                basic::i32 bucket_index = this.get_bucket_index(key);
                entry* current = &this.buckets[bucket_index];
                
                while (current is !void)
                {
                    if (current.is_occupied and (*this.compare_func)(current.key, key))
                    {
                        return current.value;
                    };
                    current = current.next;
                };
                
                return void;
            };
            
            def contains_key(void* key) -> bool
            {
                return this.get(key) is !void;
            };
            
            def count() -> basic::i32
            {
                return this.size;
            };
            
            def is_empty() -> bool
            {
                return this.size == 0;
            };
        };
        
        // ================================================================
        // LINKED LIST
        // ================================================================
        
        object list
        {
            object node
            {
                void* adata;
                node* next;
                node* prev;
                
                def __init() -> this
                {
                    this.adata = void;
                    this.next = void;
                    this.prev = void;
                    return this;
                };
                
                def __exit() -> void
                {
                    return void;
                };
            };
            
            node* head;
            node* tail;
            basic::i32 size;
            basic::ui32 element_size;
            
            def __init(basic::ui32 elem_size) -> this
            {
                this.head = void;
                this.tail = void;
                this.size = 0;
                this.element_size = elem_size;
                return this;
            };
            
            def __exit() -> void
            {
                this.clear();
                return void;
            };
            
            def push_front(void* element) -> bool
            {
                node* new_node = (node*)api::allocate_memory(sizeof(node));
                if (new_node is void) { return false; };
                
                *new_node = node();
                new_node->data = api::allocate_memory(this.element_size);
                if (new_node->data is void)
                {
                    api::deallocate_memory(new_node, sizeof(node));
                    return false;
                };
                
                // Copy element data
                basic::ui8* src = (basic::ui8*)element;
                basic::ui8* dst = (basic::ui8*)new_node->data;
                for (basic::ui32 i = 0; i < this.element_size; i++)
                {
                    dst[i] = src[i];
                };
                
                if (this.head is void)
                {
                    this.head = new_node;
                    this.tail = new_node;
                }
                else
                {
                    new_node.next = this.head;
                    this.head.prev = new_node;
                    this.head = new_node;
                };
                
                this.size++;
                return true;
            };
            
            def push_back(void* element) -> bool
            {
                node* new_node = (node*)api::allocate_memory(sizeof(node));
                if (new_node is void) { return false; };
                
                *new_node = node();
                new_node->data = api::allocate_memory(this.element_size);
                if (new_node->data is void)
                {
                    api::deallocate_memory(new_node, sizeof(node));
                    return false;
                };
                
                // Copy element data
                basic::ui8* src = (basic::ui8*)element;
                basic::ui8* dst = (basic::ui8*)new_node->data;
                for (basic::ui32 i = 0; i < this.element_size; i++)
                {
                    dst[i] = src[i];
                };
                
                if (this.tail is void)
                {
                    this.head = new_node;
                    this.tail = new_node;
                }
                else
                {
                    this.tail.next = new_node;
                    new_node.prev = this.tail;
                    this.tail = new_node;
                };
                
                this.size++;
                return true;
            };
            
            def pop_front(void* out_element) -> bool
            {
                if (this.head is void) { return false; };
                
                node* old_head = this.head;
                
                // Copy data out
                if (out_element is !void)
                {
                    basic::ui8* src = (basic::ui8*)old_head.adata;
                    basic::ui8* dst = (basic::ui8*)out_element;
                    for (basic::ui32 i = 0; i < this.element_size; i++)
                    {
                        dst[i] = src[i];
                    };
                };
                
                this.head = old_head.next;
                if (this.head is !void)
                {
                    this.head.prev = void;
                }
                else
                {
                    this.tail = void;
                };
                
                api::deallocate_memory(old_head.adata, this.element_size);
                api::deallocate_memory(old_head, sizeof(node));
                this.size--;
                return true;
            };
            
            def pop_back(void* out_element) -> bool
            {
                if (this.tail is void) { return false; };
                
                node* old_tail = this.tail;
                
                // Copy data out
                if (out_element is !void)
                {
                    basic::ui8* src = (basic::ui8*)old_tail.adata;
                    basic::ui8* dst = (basic::ui8*)out_element;
                    for (basic::ui32 i = 0; i < this.element_size; i++)
                    {
                        dst[i] = src[i];
                    };
                };
                
                this.tail = old_tail.prev;
                if (this.tail is !void)
                {
                    this.tail.next = void;
                }
                else
                {
                    this.head = void;
                };
                
                api::deallocate_memory(old_tail.adata, this.element_size);
                api::deallocate_memory(old_tail, sizeof(node));
                this.size--;
                return true;
            };
            
            def length() -> basic::i32
            {
                return this.size;
            };
            
            def is_empty() -> bool
            {
                return this.size == 0;
            };
            
            def clear() -> void
            {
                while (this.head is !void)
                {
                    node* next = this.head.next;
                    api::deallocate_memory(this.head.adata, this.element_size);
                    api::deallocate_memory(this.head, sizeof(node));
                    this.head = next;
                };
                this.tail = void;
                this.size = 0;
                return void;
            };
        };
        
        // ================================================================
        // CONVENIENCE HASH FUNCTIONS
        // ================================================================
        
        namespace hash_functions
        {
            def hash_int(void* key) -> basic::ui32
            {
                int* int_key = (int*)key;
                return (basic::ui32)(*int_key * 2654435761);
            };
            
            def hash_string(void* key) -> basic::ui32
            {
                basic::string* str_key = (basic::string*)key;
                basic::ui32 hash_value = 5381;
                basic::string key_str = str_key.__expr();
                const char* str = (const char*)key_str;
                
                for (basic::i32 i = 0; i < str_key.len(); i++)
                {
                    hash_value = ((hash_value << 5) + hash_value) + (basic::ui32)str[i];
                };
                
                return hash_value;
            };
            
            def hash_float(void* key) -> basic::ui32
            {
                float* float_key = (float*)key;
                basic::ui32* bits = (basic::ui32*)float_key;
                return *bits;
            };
        };
        
        // ================================================================
        // CONVENIENCE COMPARISON FUNCTIONS
        // ================================================================
        
        namespace compare_functions
        {
            def compare_int(void* a, void* b) -> bool
            {
                int* int_a = (int*)a;
                int* int_b = (int*)b;
                return *int_a == *int_b;
            };
            
            def compare_string(void* a, void* b) -> bool
            {
                basic::string* str_a = (basic::string*)a;
                basic::string* str_b = (basic::string*)b;
                return str_a.__ee(*str_b);
            };
            
            def compare_float(void* a, void* b) -> bool
            {
                float* float_a = (float*)a;
                float* float_b = (float*)b;
                return *float_a == *float_b;
            };
        };
    };
};