NAME		= webServer

SRC_DIR     = src/
OBJ_DIR     = obj/
INC_DIR     = includes/

INCLUDES	= -I$(INC_DIR)

# Encuentra automáticamente todos los archivos .cpp en src/ y subdirectorios
SRCS_DIR	= $(shell find $(SRC_DIR) -name '*.cpp')
# main.cpp en la raíz del proyecto
MAIN_SRC	= main.cpp
# Combina todos los archivos fuente
SRCS		= $(MAIN_SRC) $(SRCS_DIR)

# Encuentra automáticamente todos los archivos .hpp en includes/ y subdirectorios
HEADERS 	= $(shell find $(INC_DIR) -name '*.hpp')

# Genera los archivos objeto manteniendo la estructura de directorios
OBJS_DIR	= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(SRCS_DIR))
MAIN_OBJ	= $(OBJ_DIR)main.o
OBJS		= $(MAIN_OBJ) $(OBJS_DIR)

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98

RM			= rm -f

all:	$(OBJ_DIR) $(NAME).out

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(dir $(OBJS))

$(NAME).out:	$(OBJS) Makefile
				$(CXX) $(CXXFLAGS) $(OBJS) -o $@
				@echo "\033[1;32m​ Compilación exitosa finalizada!!!!!​\033[0m"

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Regla especial para compilar main.cpp desde la raíz
$(OBJ_DIR)main.o: $(MAIN_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)

fclean:		clean
			$(RM) $(NAME).out

re:			fclean all

.PHONY:		all clean fclean re
