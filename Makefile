PATH = C:\Users\Public\winkey.exe

all: svc tinky winkey

svc.obj:
	cl.exe /WX /Wall ./tinky-winkey/sourcessvc.cpp

tinky.obj:
	cl.exe /WX /Wall ./tinky-winkey/sourcestinky.cpp

winkey.obj:
	cl.exe /WX /Wall ./tinky-winkey/sources/winkey.cpp

svc: svc.obj
	link.exe ./svc.obj /OUT:svc.exe

tinky: tinky.obj
	link.exe ./tinky.obj /OUT:tinky.exe

winkey: winkey.obj
	link.exe ./winkey.obj /OUT:$(PATH)

clean:
	del svc.obj
	del tinky.obj
	del winkey.obj
	
fclean: clean
	del svc.exe
	del tinky.exe
	del $(PATH)

re: fclean all