def decrypt_str(src):
        res=''
        src=list(map(ord,list(src)))
        for te in src:
	if 105<= te <= 112: te+=9
	elif 114 <= c <= 121: te-=9
	elif 73 <= te <= 80: te+=9
	elif 82 <= te <= 89: te-=9
                res+=chr(te)
        return res
def get_string(addr):
        out=b''
        while True:
                if get_wide_byte(addr)!=0:
                        out+=chr(get_wide_byte(addr))
                else:
                        break
                addr+=1
        return out

def find_function_arg(addr):
        while True:
                addr=idc.prev_head(addr)
                if print_insn_mnem(addr)[:4]=="push":
                        return get_operand_value(addr,0)
def main():
        for x in XrefsTo(0x10003B00,flags=0):
                ref=find_function_arg(x.frm)
                string=get_string(ref)
                print(string)
                decstr=decrypt_str(string)
                print('[STRING]:%s\n[Deobfuscated]:%s' % (string,decstr))
                set_cmt(x.frm,decstr,0)
                set_cmt(ref,decstr,0)

                ct=idaapi.decompile(x.frm)
                tl=idaapi.treeloc_t()
                tl.ea=x.frm
                tl.itp=idaapi.ITP_SEMI
                ct.set_user_cmt(tl,decstr)
                ct.save_user_cmts()

if __name__=='__main__':
        main() 