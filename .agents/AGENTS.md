# C++ Coding Rules

## Coding Style
- Follow the Chromium C++ Coding Style.

## Exception Handling
- C++ `try/catch` blocks are strictly BANNED. Do not write `try` or `catch` in any C++ source or header files in the production codebase.
- Use error-checking return values, non-throwing overloads (such as `std::filesystem::last_write_time` taking `std::error_code`), and parser checks (like `std::from_chars`) instead of throwing exceptions or using `try/catch`.
