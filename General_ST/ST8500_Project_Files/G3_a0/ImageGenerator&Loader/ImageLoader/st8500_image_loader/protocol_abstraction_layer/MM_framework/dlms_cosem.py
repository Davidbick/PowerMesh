from bytes_op import ByteOp


class QuarterHour:
    profile_energy_elem_num = 0x02
    profile_energy = []
    profile_power_inst = []
    profile_power_avg = []
    profile_tot_energy = []

    def __init__(self, energy, power_inst, power_avg, tot_energy):
        # 4 bytes
        self.profile_energy = ByteOp.uint32_to_bytes(energy)
        # 2 bytes
        self.profile_power_inst = ByteOp.uint16_to_bytes(power_inst)
        # 2 bytes
        self.profile_power_avg = ByteOp.uint16_to_bytes(power_avg)
        # 4 bytes
        self.profile_tot_energy = ByteOp.uint32_to_bytes(tot_energy)

    @staticmethod
    def id():
        return 0x01

    def build_array(self):
        content = [self.profile_energy_elem_num]
        content.extend(self.profile_energy)
        content.extend(self.profile_power_inst)
        content.extend(self.profile_power_avg)
        content.extend(self.profile_tot_energy)

        return content

    def __len__(self):
        return len(self.build_array())

    def __iter__(self):
        return iter(self.build_array())

    def __str__(self):
        return str(list(self))


class DlmsCosemFrame:
    class SecurityHeader:
        security_ctrl = 0x30
        invocation_counter = 0x00000000

        def build_array(self):
            content = [self.security_ctrl]
            content.extend(ByteOp.uint32_to_bytes(self.invocation_counter))

            return content

        def __len__(self):
            return len(self.build_array())

        def __iter__(self):
            return iter(self.build_array())

        def __str__(self):
            return str(list(self))

    class CypherText:
        class CompactBuffer:
            id = 0x00
            timestamp_packet_generation = 0x00000000
            timestamp_measure = 0x00000000
            current_timezone = 0x00
            payload = []

            def build_array(self):
                content = [self.id]
                content.extend(ByteOp.uint32_to_bytes(self.timestamp_packet_generation))
                content.extend(ByteOp.uint32_to_bytes(self.timestamp_measure))
                content.append(self.current_timezone)
                content.extend(self.payload)

                return content

            def __len__(self):
                return len(self.build_array())

            def __iter__(self):
                return iter(self.build_array())

            def __str__(self):
                return str(list(self))

        tag = 0x0F
        long_invoke_id_and_priority = 0x00000000
        date_time = 0x00
        compact_data_buffer_type = 0x09
        compact_data_buffer_len = 0x17
        comp_buff = CompactBuffer()

        def build_array(self):
            content = [self.tag]
            content.extend(ByteOp.uint32_to_bytes(self.long_invoke_id_and_priority))
            content.append(self.date_time)
            content.append(self.compact_data_buffer_type)
            content.append(self.compact_data_buffer_len)
            content.extend(self.comp_buff)

            return content

        def __len__(self):
            return len(self.build_array())

        def __iter__(self):
            return iter(self.build_array())

        def __str__(self):
            return str(list(self))

    class T:
        authentication_tag = 0x00000000

        def build_array(self):
            content = ByteOp.uint32_to_bytes(self.authentication_tag)

            return content

        def __len__(self):
            return len(self.build_array())

        def __iter__(self):
            return iter(self.build_array())

        def __str__(self):
            return str(list(self))

    tag = 0xDB
    sys_title = 0x00
    length = 0x30
    security_header = SecurityHeader()
    cypher_text = CypherText()
    t = T()

    def build_array(self):
        content = [self.tag, self.sys_title, self.length]
        content.extend(self.security_header)
        content.extend(self.cypher_text)
        content.extend(self.t)

        return content

    def __len__(self):
        return len(self.build_array())

    def __iter__(self):
        return iter(self.build_array())

    def __str__(self):
        return str(list(self))

    def set_quarter_hour(self, energy, power_inst, power_avg, tot_energy):
        quarter_hour = QuarterHour(energy, power_inst, power_avg, tot_energy)
        self.cypher_text.comp_buff.id = QuarterHour.id()
        self.cypher_text.comp_buff.payload = list(quarter_hour)

    def increase_invocation_counter(self):
        self.security_header.invocation_counter += 1


# Testing
if __name__ == '__main__':
    p = DlmsCosemFrame()

    print(len(p))
    print(p)
