#ifndef MODE_HPP
#define MODE_HPP

enum class Mode { OFF, COOL, HEAT, AUTO };

const char* mode_to_str(Mode mode);
Mode str_to_mode(const char* str);

#endif
