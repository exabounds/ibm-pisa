extern void ciao(int r);

#define LENGTH 100

int main(int argc,char* argv[]){
	int l[LENGTH];
	int res=0;
	int* i;
	
	for(i=l;i-l<LENGTH;i++){
		*i = i-l;
		res = res +*i;
	}
	
	for(i=i;i!=l;i--){
		res = res - *i;
	}
	
	res = res - *i;
	
	ciao(res);
	
	return res;
}
