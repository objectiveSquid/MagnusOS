# Code of conduct
## C/C++
- When building in any configuration (debug, test or release), there should be **no warnings or errors**. This is ensured with `-Wall` and `-Werror`.
- **Do not use newline braces** where you can avoid it.
- **Indentation consists of 4 spaces**, never a tab.
- **There should never be an if/else statement longer than 3 cases**, if it can be a switch statement instead.
- **Every file should end with an empty newline**.

## Python/SConscripts
- **Indentation consists of 4 spaces**, never a tab.
- **There should never be an if/else statement longer than 3 cases**, if it can be a match statement instead.
- **Every file should end with an empty newline**.
- If you can **type hint with builtin types**, do so. And try to type hint everything you can.
