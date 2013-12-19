# Tommy Steimel

CXX	= g++

NAME	= fat
OFILES	= directory.o fileSystem.o $(NAME).o

.SUFFIXES: .o .cpp

 all:	$(NAME)

 $(NAME): $(OFILES)
	$(CXX) $(OFILES) -o $(NAME)

 $(NAME).o: $(NAME).cpp
	$(CXX) -c -o $(NAME).o $(NAME).cpp

fileSystem.o: fileSystem.cpp
	$(CXX) -c -o fileSystem.o fileSystem.cpp

directory.o: directory.cpp
	$(CXX) -c -o directory.o directory.cpp

 clean:
	-rm *.o $(NAME)