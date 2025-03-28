object test {
	char y = "First";
};

struct
{
	char z = "TEST STRING";
} MyStruct;

namespace myNS
{
	class test
	{
	};
	
	class test3
	{
	};
};

char foo()
{
	print(input("Print before return: "), "\n");
	return "The^Fifth.";
};

int main()
{
	test{} a;
	MyNS.test.test2{} obj;
	MyNS.test3.test4{} obj2;
	print(obj.o, "\n");
	print(obj2.o, "\n");
	
	if (obj2.x > obj2.y)
	{
		print("Less than");
	} else {
		print("Greater than");
	};
	
	int abc = 123;
	char xyz = "789";
	
	print(abc,xyz,"\n");
	
	print(i"{}\n{}\n{}\n{}\n":{a.y + " Second!";
				 a.y + " Third!";
				 "Fourth!";
				 foo()[3];
				 });
	
	return 0;
};
