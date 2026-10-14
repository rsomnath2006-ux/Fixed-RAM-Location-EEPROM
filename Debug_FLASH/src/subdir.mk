################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/nvm\ demo.c 

OBJS += \
./src/main.o \
./src/nvm\ demo.o 

C_DEPS += \
./src/main.d \
./src/nvm\ demo.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/main.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/nvm\ demo.o: ../src/nvm\ demo.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@src/nvm demo.args" -MMD -MP -MF"src/nvm demo.d" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


