all: svc tinky winkey

svc.obj:
  cl.exe /WX /Wall .\tinky-winky\sources\svc.cpp

tinky.obj:
  cl.exe /WX /Wall .\tinky-winky\sources\tinky.cpp

winkey.obj:
	cl.exe /WX /Wall .\tinky-winky\sources\winkey.cpp



svc: svc.obj
	link.exe ./svc.obj /OUT:svc.exe

tinky: tinky.obj
	link.exe ./tinky.obj /OUT:tinky.exe

winkey: winkey.obj
	link.exe ./winkey.obj /OUT:winkey.exe



clean:
	del svc.obj
	del tinky.obj
	del winkey.obj
	
fclean: clean
	del svc.exe
	del tinky.exe
	del winkey.exe

re: fclean all