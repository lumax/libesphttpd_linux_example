SECTIONS
{
        .rodata : ALIGN(4) SUBALIGN(4) {
                webpages_espfs_start = .;
                *(*)
                webpages_espfs_end = .;
                webpages_espfs_size = webpages_espfs_end - webpages_espfs_start;
        }
}

