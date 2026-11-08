class ByteOp:

    @staticmethod
    def uint32_to_bytes(integer, little_endian=False) -> list:
        if little_endian:
            ret = [((integer >> 24) & 0xFF), ((integer >> 16) & 0xFF), ((integer >> 8) & 0xFF), (integer & 0xFF)]
        else:
            ret = [(integer & 0xFF), ((integer >> 8) & 0xFF), ((integer >> 16) & 0xFF), ((integer >> 24) & 0xFF)]
        return ret

    @staticmethod
    def uint16_to_bytes(integer, little_endian=False) -> list:
        if little_endian:
            ret = [((integer >> 8) & 0xFF), (integer & 0xFF)]
        else:
            ret = [(integer & 0xFF), ((integer >> 8) & 0xFF)]
        return ret

    @staticmethod
    def bytes_to_uint32(byte_list, little_endian=False) -> int:
        if little_endian:
            ret = (byte_list[0] << 24) | (byte_list[1] << 16) | (byte_list[2] << 8) | byte_list[3]
        else:
            ret = (byte_list[3] << 24) | (byte_list[2] << 16) | (byte_list[1] << 8) | byte_list[0]
        return ret

    @staticmethod
    def bytes_to_uint16(byte_list, little_endian=False) -> int:
        if little_endian:
            ret = (byte_list[0] << 8) | byte_list[1]
        else:
            ret = (byte_list[1] << 8) | byte_list[0]
        return ret
