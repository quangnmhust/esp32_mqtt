void func(void);

typedef struct {
	    int id;
	    int pbgt;
	    float ec;
	    float ph;
	    float od;
	    float teamp;
	    int year ;
	    int mon ;
	    int mday ;
	    int hour ;
	    int minz ;
	    int sec ;
		int lenth;
} SensorData;

void encodeDataToHex( int id, int pbgt,float ec, float od, float ph, float teamp, int year ,int mon ,int mday ,int hour ,int min ,int sec , char *hexString);

void decodeHexToData(const char *hexString, size_t hexStringLen, SensorData *data);

void GPIO_SET_UART();