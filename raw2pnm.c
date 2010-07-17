#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

off_t file_size(const char *path) {
    struct stat st;
    stat(path, &st);
    return st.st_size;
}

void convert_ppm(unsigned int width, unsigned int height,
        FILE *raw_fp, char *name) {
    strcpy(strrchr(name, '.'), ".ppm");
    FILE *ppm_fp = fopen(name, "wb");
    fprintf(stderr, "Writing %s\n", name);
    fprintf(ppm_fp, "P6\n%d %d\n255\n", width, height);

    char h[3], r[width], g[width], b[width];
    for(int i = 0; i < height; i++) {
        fread(h, 1, 3, raw_fp); fread(r, 1, width, raw_fp);
        fread(h, 1, 3, raw_fp); fread(g, 1, width, raw_fp);
        fread(h, 1, 3, raw_fp); fread(b, 1, width, raw_fp);
        for(int j = 0; j < width; j++) {
            fputc(r[j], ppm_fp);
            fputc(g[j], ppm_fp);
            fputc(b[j], ppm_fp);
        }
    }
    fclose(ppm_fp);
}

void convert_pgm(unsigned int width, unsigned int height,
        FILE *raw_fp, char *name) {
    strcpy(strrchr(name, '.'), ".pgm");
    FILE *pgm_fp = fopen(name, "wb");
    fprintf(stderr, "Writing %s\n", name);
    fprintf(pgm_fp, "P5\n%d %d\n255\n", width, height);

    char h[3], g[width];
    for(int i = 0; i < height; i++) {
        fread(h, 1, 3, raw_fp);
        fread(g, 1, width, raw_fp);
        fwrite(g, 1, width, pgm_fp);
    }
    fclose(pgm_fp);
}

int main(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        char *name = argv[i];
        FILE *raw_fp = fopen(name, "rb");

        unsigned char buf[3];
        fread(buf, sizeof(unsigned char), 3, raw_fp);
        unsigned char type = buf[0];
        unsigned int width = (buf[2] << 8) + buf[1];
        unsigned int height = file_size(name) / (width + 3);

        if(strchr("DHL", type)) /* color */
            convert_ppm(width, height/3, raw_fp, name);
        else if(type == '@') /* gray */
            convert_pgm(width, height, raw_fp, name);
        else {
            fprintf(stderr, "ERROR: '%s' has unrecognised type '%c'\n",
                    name, type);
            continue;
        }
    }
    return 0;
}
