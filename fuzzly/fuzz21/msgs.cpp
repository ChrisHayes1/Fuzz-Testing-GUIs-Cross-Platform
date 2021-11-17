struct SockMsgAuth {
    enum endianness endian;
    unsigned short protocol_major_version;
    unsigned short protocol_minor_version;
    unsigned short  auth_name_len;
    unsigned short auth_data_len;
};