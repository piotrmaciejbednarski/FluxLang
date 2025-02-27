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
		object test2
		{
			char o = "TEST STRING #2";
		};
	};
	
	class test3
	{
		object  test4
		{
			char o = "TEST STRING #3";
		};
	};
};

char foo()
{
	print(input("Print before return: "));
	return "The^Fifth.";
};

int main()
{
	test{} a;
	MyNS::test::test2{} obj;
	MyNS::test3::test4{} obj2;
	print(obj.o);
	print(obj2.o);
	
	print(i"{}\n{}\n{}\n{}":{a.y + " Second!";
				 a.y + " Third!";
				 "Fourth!";
				 foo()[3];
				 });
	
	return 0;
};
