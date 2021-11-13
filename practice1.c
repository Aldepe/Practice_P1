/**
 * @file   practice1.c
 * @author Alvaro Delgado Perez
 * @date   14 October 2021
 * @brief  A kernel module for controlling 2 GPIO LED with 2 button pair. The device mounts devices via
 * sysfs /sys/class/gpio/gpio115 and gpio49. Therefore, this test LKM circuit assumes that an LED
 * is attached to GPIO 49 which is on P9_23 and the button is attached to GPIO 115 on P9_27. There
 * is no requirement for a custom overlay, as the pins are in their default mux mode states.
*/
 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alvaro Delgado Perez");
MODULE_DESCRIPTION("A Button/LED test driver for the BBB");
MODULE_VERSION("0.1");
 
static unsigned int gpioLED1st = 20;        ///< hard coding the LED gpio for this example to GPIO20
static unsigned int gpioLED2nd = 16;        ///< hard coding the LED gpio for this example to GPIO16
static unsigned int gpioButtonA = 26;       ///< hard coding the button gpio for this example to GPIO26
static unsigned int gpioButtonB = 19;       ///< hard coding the button gpio for this example to GPIO19
static unsigned int gpioButtonC = 13;       ///< hard coding the button gpio for this example to GPIO13
static unsigned int gpioButtonD = 21;       ///< hard coding the button gpio for this example to GPIO21
static unsigned int irqNumberA;             ///< Used to share the IRQ number within this file
static unsigned int irqNumberB;             ///< Used to share the IRQ number within this file
static unsigned int irqNumberC;             ///< Used to share the IRQ number within this file
static unsigned int irqNumberD;             ///< Used to share the IRQ number within this file
static unsigned int numberPressesA = 0;     ///< For information, store the number of button presses
static unsigned int numberPressesB = 0;     ///< For information, store the number of button presses
static unsigned int numberPressesC = 0;     ///< For information, store the number of button presses
static unsigned int numberPressesD = 0;     ///< For information, store the number of button presses
 
/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handlerA(unsigned int irq, void *dev_id, struct pt_regs *regs);
/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handlerB(unsigned int irq, void *dev_id, struct pt_regs *regs);
/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handlerC(unsigned int irq, void *dev_id, struct pt_regs *regs);
/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handlerD(unsigned int irq, void *dev_id, struct pt_regs *regs);
 
/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbgpio_init(void){
    int result = 0;
    printk(KERN_INFO "GPIO_TEST: Initializing the GPIO_TEST LKM\n");
    // Is the GPIO a valid GPIO number (e.g., the BBB has 4x32 but not all available)
    if (!gpio_is_valid(gpioLED1st) || !gpio_is_valid(gpioLED2nd)){
        printk(KERN_INFO "GPIO_TEST: invalid LED GPIO\n");
        return -ENODEV;
    }
    // Going to set up the LED. It is a GPIO in output mode and will be on by default

    gpio_request(gpioLED1st, "sysfs");              // gpioLED is hardcoded to 20, request it
    gpio_direction_output(gpioLED1st, true);        // Set the gpio to be in output mode and on
    gpio_export(gpioLED1st, false);                 // Causes gpio49 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed
    gpio_request(gpioLED2nd, "sysfs");              // gpioLED is hardcoded to 16, request it
    gpio_direction_output(gpioLED2nd, true);        // Set the gpio to be in output mode and on
    gpio_export(gpioLED2nd, false);                 // Causes gpio49 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed                  
    gpio_request(gpioButtonA, "sysfs");             // Set up the gpioButton
    gpio_direction_input(gpioButtonA);              // Set the button GPIO to be an input
    gpio_set_debounce(gpioButtonA, 1000);            // Debounce the button with a delay of 1000ms
    gpio_export(gpioButtonA, false);                 // Causes gpio115 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed
                                                    // Perform a quick test to see that the button is working as expected on LKM load
    gpio_request(gpioButtonB, "sysfs");             // Set up the gpioButton
    gpio_direction_input(gpioButtonB);              // Set the button GPIO to be an input
    gpio_set_debounce(gpioButtonB, 1000);            // Debounce the button with a delay of 1000ms
    gpio_export(gpioButtonB, false);                // Causes gpio115 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed
                                                    // Perform a quick test to see that the button is working as expected on LKM load
    gpio_request(gpioButtonC, "sysfs");             // Set up the gpioButton
    gpio_direction_input(gpioButtonC);              // Set the button GPIO to be an input
    gpio_set_debounce(gpioButtonC, 1000);            // Debounce the button with a delay of 1000ms
    gpio_export(gpioButtonC, false);                // Causes gpio115 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed
                                                    // Perform a quick test to see that the button is working as expected on LKM load
    gpio_request(gpioButtonD, "sysfs");             // Set up the gpioButton
    gpio_direction_input(gpioButtonD);              // Set the button GPIO to be an input
    gpio_set_debounce(gpioButtonD, 1000);            // Debounce the button with a delay of 1000ms
    gpio_export(gpioButtonD, false);                // Causes gpio115 to appear in /sys/class/gpio
                                                    // the bool argument prevents the direction from being changed
                                                    // Perform a quick test to see that the button is working as expected on LKM load
    
    printk(KERN_INFO "GPIO_TEST: The button A state is currently: %d\n", gpio_get_value(gpioButtonA));
    printk(KERN_INFO "GPIO_TEST: The button B state is currently: %d\n", gpio_get_value(gpioButtonB));
    printk(KERN_INFO "GPIO_TEST: The button C state is currently: %d\n", gpio_get_value(gpioButtonC));
    printk(KERN_INFO "GPIO_TEST: The button D state is currently: %d\n", gpio_get_value(gpioButtonD));
 
    // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
    irqNumberA = gpio_to_irq(gpioButtonA);
    printk(KERN_INFO "GPIO_TEST: The button A is mapped to IRQ: %d\n", irqNumberA);
    // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
    irqNumberB = gpio_to_irq(gpioButtonB);
    printk(KERN_INFO "GPIO_TEST: The button B is mapped to IRQ: %d\n", irqNumberB);
    // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
    irqNumberC = gpio_to_irq(gpioButtonC);
    printk(KERN_INFO "GPIO_TEST: The button C is mapped to IRQ: %d\n", irqNumberC);
    // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
    irqNumberD = gpio_to_irq(gpioButtonD);
    printk(KERN_INFO "GPIO_TEST: The button D is mapped to IRQ: %d\n", irqNumberD);
    
    // This next call requests an interrupt line
    result = request_irq(irqNumberA,             // The interrupt number requested
                            (irq_handler_t) ebbgpio_irq_handlerA, // The pointer to the handler function below
                            IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                            "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                            NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
    
    printk(KERN_INFO "GPIO_TEST: The interrupt A request result is: %d\n", result);
    // This next call requests an interrupt line
    result = request_irq(irqNumberB,             // The interrupt number requested
                            (irq_handler_t) ebbgpio_irq_handlerB, // The pointer to the handler function below
                            IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                            "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                            NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
    
    printk(KERN_INFO "GPIO_TEST: The interrupt B request result is: %d\n", result);
    // This next call requests an interrupt line
    result = request_irq(irqNumberC,             // The interrupt number requested
                            (irq_handler_t) ebbgpio_irq_handlerC, // The pointer to the handler function below
                            IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                            "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                            NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
    
    printk(KERN_INFO "GPIO_TEST: The interrupt C request result is: %d\n", result);
    // This next call requests an interrupt line
    result = request_irq(irqNumberD,             // The interrupt number requested
                            (irq_handler_t) ebbgpio_irq_handlerD, // The pointer to the handler function below
                            IRQF_TRIGGER_RISING,   // Interrupt on rising edge (button press, not release)
                            "ebb_gpio_handler",    // Used in /proc/interrupts to identify the owner
                            NULL);                 // The *dev_id for shared interrupt lines, NULL is okay
    
    printk(KERN_INFO "GPIO_TEST: The interrupt D request result is: %d\n", result);
    return result;
}
 
/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *  GPIOs and display cleanup messages.
 */

static void __exit ebbgpio_exit(void){
    	printk(KERN_INFO "GPIO_TEST: The button A state is currently: %d\n", gpio_get_value(gpioButtonA));
    	printk(KERN_INFO "GPIO_TEST: The button B state is currently: %d\n", gpio_get_value(gpioButtonB));
    	printk(KERN_INFO "GPIO_TEST: The button C state is currently: %d\n", gpio_get_value(gpioButtonC));
    	printk(KERN_INFO "GPIO_TEST: The button D state is currently: %d\n", gpio_get_value(gpioButtonD));
    	printk(KERN_INFO "GPIO_TEST: The button A was pressed %d times\n", numberPressesA);
    	printk(KERN_INFO "GPIO_TEST: The button B was pressed %d times\n", numberPressesB);
    	printk(KERN_INFO "GPIO_TEST: The button C was pressed %d times\n", numberPressesC);
    	printk(KERN_INFO "GPIO_TEST: The button D was pressed %d times\n", numberPressesD);
    	gpio_set_value(gpioLED1st, 0);              // Turn the LED off, makes it clear the device was unloaded
    	gpio_unexport(gpioLED1st);                  // Unexport the LED GPIO
    	gpio_set_value(gpioLED2nd, 0);              // Turn the LED off, makes it clear the device was unloaded
    	gpio_unexport(gpioLED2nd);                  // Unexport the LED GPIO
    	free_irq(irqNumberA, NULL);               // Free the IRQ number, no *dev_id required in this case
    	free_irq(irqNumberB, NULL);               // Free the IRQ number, no *dev_id required in this case
    	free_irq(irqNumberC, NULL);               // Free the IRQ number, no *dev_id required in this case
    	free_irq(irqNumberD, NULL);               // Free the IRQ number, no *dev_id required in this case
    	gpio_unexport(gpioButtonA);               // Unexport the Button GPIO
    	gpio_unexport(gpioButtonB);               // Unexport the Button GPIO
    	gpio_unexport(gpioButtonC);               // Unexport the Button GPIO
    	gpio_unexport(gpioButtonD);               // Unexport the Button GPIO
    	gpio_free(gpioLED1st);                      // Free the LED GPIO
    	gpio_free(gpioLED2nd);                      // Free the LED GPIO
    	gpio_free(gpioButtonA);                   // Free the Button GPIO
    	gpio_free(gpioButtonB);                   // Free the Button GPIO
    	gpio_free(gpioButtonC);                   // Free the Button GPIO
    	gpio_free(gpioButtonD);                   // Free the Button GPIO
    	printk(KERN_INFO "GPIO_TEST: Goodbye from the LKM!\n");
}
 
/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq_handlerA(unsigned int irq, void *dev_id, struct pt_regs *regs){
    	static char* argv[]=  {"/usr/bin/scripts/1stscript.sh", NULL};
       	static char* envp[] = {"HOME=/", NULL};
	gpio_set_value(gpioLED1st, true);          // Set the physical LED accordingly
    	printk(KERN_INFO "GPIO_TEST: Interrupt! (button A state is %d)\n", gpio_get_value(gpioButtonA));
    	numberPressesA++;                         // Global counter, will be outputted when the module is unloaded
    	call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}
 
/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq_handlerB(unsigned int irq, void *dev_id, struct pt_regs *regs){
    	static char* argv[]=  {"/usr/bin/scripts/2ndscript.sh", NULL};
    	static char* envp[] = {"HOME=/", NULL};
	gpio_set_value(gpioLED1st, false);          // Set the physical LED accordingly
    	printk(KERN_INFO "GPIO_TEST: Interrupt! (button B state is %d)\n", gpio_get_value(gpioButtonB));
    	numberPressesB++;                         // Global counter, will be outputted when the module is unloaded
	call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}
 
/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq_handlerC(unsigned int irq, void *dev_id, struct pt_regs *regs){
    	static char* argv[]=  {"/usr/bin/scripts/3rdscript.sh", NULL};
        static char* envp[] = {"HOME=/", NULL};
	gpio_set_value(gpioLED2nd, true);          // Set the physical LED accordingly
    	printk(KERN_INFO "GPIO_TEST: Interrupt! (button C state is %d)\n", gpio_get_value(gpioButtonC));
    	numberPressesC++;                         // Global counter, will be outputted when the module is unloaded
    	call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq_handlerD(unsigned int irq, void *dev_id, struct pt_regs *regs){
    	static char* argv[]=  {"/usr/bin/scripts/4thscript.sh", NULL};
        static char* envp[] = {"HOME=/", NULL};
	gpio_set_value(gpioLED2nd, false);          // Set the physical LED accordingly
    	printk(KERN_INFO "GPIO_TEST: Interrupt! (button D state is %d)\n", gpio_get_value(gpioButtonD));
    	numberPressesD++;                         // Global counter, will be outputted when the module is unloaded
    	call_usermodehelper(argv[0], argv, envp, UMH_NO_WAIT);
    	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}
 
/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
