################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/App.c \
../source/I2C.c \
../source/can.c \
../source/cqueue.c \
../source/fsm.c \
../source/gpio.c \
../source/pisr.c \
../source/protocol.c \
../source/sensor.c \
../source/serial.c \
../source/station.c \
../source/timer.c \
../source/uart.c 

C_DEPS += \
./source/App.d \
./source/I2C.d \
./source/can.d \
./source/cqueue.d \
./source/fsm.d \
./source/gpio.d \
./source/pisr.d \
./source/protocol.d \
./source/sensor.d \
./source/serial.d \
./source/station.d \
./source/timer.d \
./source/uart.d 

OBJS += \
./source/App.o \
./source/I2C.o \
./source/can.o \
./source/cqueue.o \
./source/fsm.o \
./source/gpio.o \
./source/pisr.o \
./source/protocol.o \
./source/sensor.o \
./source/serial.o \
./source/station.o \
./source/timer.o \
./source/uart.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -D__USE_CMSIS -DDEBUG -I../source -I../ -I../SDK/CMSIS -I../SDK/startup -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/App.d ./source/App.o ./source/I2C.d ./source/I2C.o ./source/can.d ./source/can.o ./source/cqueue.d ./source/cqueue.o ./source/fsm.d ./source/fsm.o ./source/gpio.d ./source/gpio.o ./source/pisr.d ./source/pisr.o ./source/protocol.d ./source/protocol.o ./source/sensor.d ./source/sensor.o ./source/serial.d ./source/serial.o ./source/station.d ./source/station.o ./source/timer.d ./source/timer.o ./source/uart.d ./source/uart.o

.PHONY: clean-source

