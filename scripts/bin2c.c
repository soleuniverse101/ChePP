#include <stdio.h>
#include <stdlib.h>

void write_h(const char *h_filename, const char *varname) {
    FILE *h = fopen(h_filename, "w");
    if (!h) {
        perror("Failed to open header file");
        exit(1);
    }
    fprintf(h, "#pragma once\n\n");
    fprintf(h, "extern unsigned char %s[];\n", varname);
    fprintf(h, "extern unsigned int %s_len;\n", varname);
    fclose(h);
}

void write_c(const char *c_filename, const char *bin_filename, const char *varname) {
    FILE *bin = fopen(bin_filename, "rb");
    if (!bin) {
        perror("Failed to open binary input");
        exit(1);
    }
    FILE *c = fopen(c_filename, "w");
    if (!c) {
        perror("Failed to open c output");
        fclose(bin);
        exit(1);
    }

    fprintf(c, "#include \"%s.h\"\n\n", varname);

    fprintf(c, "unsigned char %s[] = {", varname);

    int byte;
    int count = 0;
    while ((byte = fgetc(bin)) != EOF) {
        if (count % 12 == 0) {
            fprintf(c, "\n  ");
        }
        fprintf(c, "0x%02x, ", (unsigned char)byte);
        count++;
    }
    fprintf(c, "\n};\n\n");
    fprintf(c, "unsigned int %s_len = %d;\n", varname, count);

    fclose(bin);
    fclose(c);
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <input binary> <output header> <output c> <target name>\n", argv[0]);
        return 1;
    }
    const char *bin_filename = argv[1];
    const char *h_filename = argv[2];
    const char *c_filename = argv[3];
    const char *varname = argv[4];

    write_h(h_filename, varname);
    write_c(c_filename, bin_filename, varname);

    return 0;
}
