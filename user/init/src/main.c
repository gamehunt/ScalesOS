int main(){
    int i = 0;
    while(1){
        i++;
        asm volatile("int $0x80");
        if(i > 10){
            int a = 1 / 0;
        }
    }
    return 0;
}