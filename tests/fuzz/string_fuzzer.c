#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/nc_string.c"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size){

        if(size<3) return 0;
        char *new_str = (char *)malloc(size+1);
        if (new_str == NULL){
                return 0;
        }
        memcpy(new_str, data, size);
        new_str[size] = '\0';

        char *new_buffer = (char *)malloc(size+1);
        if (new_buffer == NULL){
                free(new_str);
                return 0;
        }

        _safe_snprintf(new_buffer, size+1, new_str);

        free(new_str);
        free(new_buffer);
        return 0;
}
