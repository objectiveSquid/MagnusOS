# Code of conduct
## For all languages
- Indentation consists of **4 spaces**, never a tab.
- There should never be an if/else statement longer than **3 cases**, if it can be a match/switch statement instead.
- Every file should end with an empty line.
- **Do not abbreviate** something if it's a normal english word.
    - Example of what **TO** name something: ***number*_of_calculations**
    - Example of what **NOT TO** name something: **user_input_*str***
- Constants/defines should be in **UPPER_SNAKE_CASE**.

## Assembly
- Everything should be in **snake_case**.

## C/C++
- When building in any configuration (debug, test or release), there should be **no warnings or errors**. This is ensured with `-Wall` and `-Werror`.
- **Do not use newline braces**.
- Variable names should be in **pascalCase**.
    - If they are global they should be prefixed with **g_**, and should instead be in **CamelCase**.
- Function names should be in **pascalCase**.
    - If a function is in a larger collection of functions, it should be prefixed with the name of the collection (*unless it's a very standard function, such as printf or memcpy*) and it should instead in **CamelCase**. (Example: `void GRAPHICS_PutPixel(...)`)
- Enum names should be in **CamelCase** and like functions should be prefixed with the name of the collection.
- Goto statements should only be used when having to deinitialize many variables in a function before returning.
    - Goto labels should be in **snake_case**.
- Only make variables as large as they need to be.
    - Example: If you know that a for loop will only iterate up to 5 times, you should use an `uint8_t`.

## Python/SConscripts
- If you can **type hint with builtin types**, do so. And try to type hint everything you can.
- Function and variable names should be in **snake_case**.
- Classes should be in **CamelCase**.
