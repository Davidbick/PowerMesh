class SignatureAlg:
    NOSIGN = 0
    RSA = 1
    ECDSA = 2


class DFU_FileGenerationMode:
    NOSIGN = 0
    RSA = 1
    ECDSA = 2
    STVENDOR = 3
    CORRUPTPAY = 4
    CORRUPTSIG = 5


class Vendor:
    class ProductId:
        SN_EMB = 0x0001
        SN_STM32 = 0x0002
        SN_STCOMET = 0x1043
        SN_STCOMET_LIBCOMPILED = 0x0043
        SN_STARCOM_LIBCOMPILED = 0x0044