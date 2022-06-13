all: svc tinky

svc.obj:
  cl.exe /WX /Wall .\tinky-winky\sources\svc.cpp

tinky.obj:
  cl.exe /WX /Wall .\tinky-winky\sources\tinky.cpp

svc: svc.obj
	link.exe ./svc.obj /OUT:svc.exe

tinky: tinky.obj
	link.exe ./tinky.obj /OUT:tinky.exe

clean:
	del svc.obj
	del tinky.obj
	
fclean: clean
	del svc.exe
	del tinky.exe

re: fclean all