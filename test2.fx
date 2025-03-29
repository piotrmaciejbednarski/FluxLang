do
{
    count += 1;
    
    // Logical operators
    if (count > 10 or this.errorCode != 0)
    {
        this.running = false;
    };
    
    // Using xor keyword
    Flag test = xor(true,false);
    
    // Using not keyword
    test = not test;
    
    // Empty statement
    ;;;
    
} while (this.running);