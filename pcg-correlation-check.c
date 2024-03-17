// Copyright 2024 Michael Niedermayer <michael-random@niedermayer.cc>
// License: GPL v2+

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define STATE_BITS   128

#define NUM_THREADS 32
#define STREAMS_PER_THREAD 32 // 17, 129
#define P_CORRECTION (4*NUM_THREADS*STREAMS_PER_THREAD*num_pairs_consiered) // the pattern bits correction is applied in pbinomial the 4 is for the used tests vs the lost cases due to mod 4

typedef unsigned __int128 STATET;
typedef uint64_t OUTT;
const uint64_t multiplier = 15750249268501108917ULL;
STATET inv_multiplier    = ((STATET)0x0CD365D2CB1A6A6C<<64) + 0x8B838D0354EAD59D;
STATET inv_multiplier_m1 = ((STATET)0x7CE06FB9FA20289E<<64) + 0x7F6050D9841169A5; // /4

static int64_t num_pairs_consiered = 0;
static int64_t stream_length;
static int topbits;

static OUTT pcg_dxsm(STATET *state, uint64_t multiplier, STATET inc) {
    *state = *state * multiplier + inc;

    OUTT h = *state >>   STATE_BITS / 2;
    h     ^= h      >>   STATE_BITS / 4;
    h     *= multiplier;
    h     ^= h      >> 3*STATE_BITS / 8;
    return h * (*state | 1);
}

static STATET step(STATET seed, STATET increment, STATET offset) {
    STATET m = multiplier;
    STATET i = increment;

    while(offset) {
        if(offset & 1)
            seed = seed*m + i;
        // smm + im + i
        i += i*m;
        m *= m;
        offset >>= 1;
    }
    return seed;
}

static STATET find_step(STATET seed1, STATET target_seed, STATET increment) {
    STATET offset = 0;
    STATET m = multiplier;
    STATET i = increment;

    for(int bit = 0; bit < 128; bit++) {
        if(((seed1 - target_seed)>>bit) & 1) {
            seed1 = seed1*m + i;
            offset += (STATET)1 << bit;
        }
        // smm + im + i
        i += i*m;
        m *= m;
    }
    return offset;
}

//point in the stream where s_1 - s_2 = 1, we compute this for clarity
static inline STATET characteristic_seed(STATET increment) {
    if ((increment & 3) == 1) {
        return (1-increment) * inv_multiplier_m1/4; // c*2¹²⁶
    } else {
        assert((increment & 3) == 3);
        return (-1-increment) * inv_multiplier_m1/4; // c*2¹²⁶
    }
}

static int compar(const STATET *a, const STATET *b){
    STATET ca = a[1];
    STATET cb = b[1];

    STATET high_mask = (0xFFFFFFFFLL>>topbits) * 0x100000001ULL;
    STATET mask = (high_mask<<64) + 0x0000000000000000;
    ca &= mask;
    cb &= mask;

    return (ca > cb) - (cb > ca);
}

/*
 * Following code is based on
 * Fast and Accurate Computation of Binomial Probabilities
    Catherine Loader
    Lucent Technologies
    Room 2C-279
    600 Mountain Avenue
    Murray Hill, NJ 07974, USA
    February 5, 2002
 */
long double stir(int64_t n)
{
#define one ((long double)1)
    const static long double low[16] = {
    0                            , 0.081061466795327258219670264,
    0.041340695955409294093822081, 0.0276779256849983391487892927,
    0.020790672103765093111522771, 0.0166446911898211921631948653,
    0.013876128823070747998745727, 0.0118967099458917700950557241,
    0.010411265261972096497478567, 0.0092554621827127329177286366,
    0.008330563433362871256469318, 0.0075736754879518407949720242,
    0.006942840107209529865664152, 0.0064089941880042070684396310,
    0.005951370112758847735624416, 0.0055547335519628013710386899
    };
    if (n<16)
        return(low[n]);
    long double nn = one / (n*(long double)n);
    if (n>500)
        return((one/12) -  (one/360)*nn)/n;
    return    ((one/12) - ((one/360) - ((one/1260) - ((one/1680) - (one/1188)*nn)*nn)*nn)*nn)/n;
}

long double bd0(int64_t x,long double np)
{
    if (fabs(x-np) < 0.1*(x+np)){
        long double ej, s, s1, v;
        int j;
        s = (x-np)*(x-np)/(x+np);
        v = (x-np)/(x+np);
        ej = 2*x*v;
        for (j=1; ;j++){
            ej *= v*v;
            s1 = s+ej/(2*j+1);
            if (s1==s)
                return(s1);
            s = s1;
        }
    }
    return x*logl(x/np) + np - x;
}

long double binomial(int64_t n,int64_t k, long double p)
{
    long double lc;
    if (k==0)
        return pow(1-p, n);
    if (k==n)
        return pow(p, n);
    lc = stir(n) - stir(k) - stir(n-k)
        - bd0(k,n*p) - bd0(n-k,n*(1.0-p));
    return expl(lc) * sqrtl(n/(one*2*M_PI*k*(n-k)));
}

long double mid_k_table[][9] = {
//This mid point sum table was build from the opposite side using the same code we use in the forward direction
//That way we can always compute along the short side at runtime (improving speed but avoid the numerical precission issues
//This table is more than accurate enough for our use case here but other use cases may require higher precission or a different algorithm
    {0},
    {0},
    {0},
{0.00000000000000000000e+00, 4.54999972134828576074e-05, 2.01414948531208896584e-04, 4.20619064740224532792e-04, 7.78584920958988818867e-03, 1.95145688460490049183e-03, 4.88176954603597501773e-04, 1.22063793440925865027e-04,3.05171706744822158902e-05},
{1.14087015390396131808e-08, 1.89917855332645180339e-06, 9.33232450756976531032e-05, 1.76555299127743228307e-03, 1.55110789712653065577e-02, 3.89910558523732954068e-03, 9.76115592468189065519e-04, 2.44112687312182567376e-04,6.10334100512584628840e-05},
{4.17874789399074263505e-09, 2.45499952962168419958e-07, 1.31885869995680197315e-04, 2.71588665121577034834e-04, 4.63881145767408059772e-04, 7.78300814610982930816e-03, 1.95127838328651839677e-03, 4.88165783620258306788e-04,1.22063095025374427480e-04},
{5.74966562532158749231e-09, 2.71656552377653133893e-07, 7.21378058648679983823e-06, 1.22696474125378420488e-04, 1.84670902200406481983e-03, 1.55054410764172466515e-02, 3.89874927924395554523e-03, 9.76093261408219032543e-04,2.44111290651581681442e-04},
{3.14793691899539569012e-09, 2.24055482242610800736e-08, 9.39996956434436145759e-07, 1.61504014955712739890e-04, 2.90187734057053695225e-04, 4.74643992371139704189e-04, 7.78229831254550582992e-03, 1.95123376476147153061e-03,4.88162990980939781976e-04},
{3.09361562790219560236e-09, 1.12945257211262592322e-08, 8.21537641643241589128e-07, 9.41878840557271570403e-06, 1.30660083621820345842e-04, 1.86689937442356091066e-03, 1.55040324580655630301e-02, 3.89866021631819783741e-03,9.76087678856116106373e-04},
{1.57704844400067275070e-09, 2.26261695215368361367e-09, 7.30813385213314675597e-08, 1.24673392519772266490e-06, 1.69427900712178535015e-04, 2.94900904750062816211e-04, 4.77331426085636731419e-04, 7.78212088115409341991e-03,1.95122261055541749298e-03},
{1.10610497956764255707e-09, 8.06216356964523433430e-10, 3.39815046126993327050e-08, 1.04740539424500206385e-06, 1.00369265381448418468e-05, 1.32689680882190281066e-04, 1.87194082297572698721e-03, 1.55036803568992922695e-02,3.89863795143489228810e-03},
{9.26442376177664891941e-10, 3.75304595779230611607e-10, 6.89303165255712989675e-09, 9.52168800858194763939e-08, 1.33436533719485320426e-06, 1.71441518424479019145e-04, 2.96083120944788406066e-04, 4.78003079863677135131e-04,7.78207652499341612289e-03},
{7.21433443512210446563e-10, 1.79727568329024254835e-10, 2.41946262754799342466e-09, 4.36603992302476320935e-08, 1.11097184413802560086e-06, 1.01958343185962551755e-05, 1.33199497014147233097e-04, 1.87320080180254754007e-03,1.55035923349459786410e-02},
{5.71508944320963760638e-10, 1.13132803314963632302e-10, 1.10763488882003845589e-09, 8.90427348576633541365e-09, 1.01555040373784912880e-07, 1.35700323808818569078e-06, 1.71946965831949315985e-04, 2.96378919605059110700e-04,4.78170980520966616783e-04},
{5.03806146146295415367e-10, 7.02316907688017597860e-11, 5.27588746757550661227e-10, 3.12038276258675046088e-09, 4.64188821248255938208e-08, 2.46776979997276057802e-07, 1.02358376113119787382e-05, 1.33327102071270926872e-04,1.87351577255871388618e-03},
{4.75003560132154968256e-10, 4.80504218769360822149e-11, 3.28808855386616896877e-10, 1.42482335973020835678e-09, 9.48067644875178202458e-09, 1.03193783675680788994e-07, 1.36270907588167621160e-06, 1.72073455409185720833e-04,2.96452884548858471488e-04},
{4.40069580145728472728e-10, 3.70899435173915318728e-11, 2.03955671929358078455e-10, 6.78420190146643792546e-10, 3.32140903574810968516e-09, 1.73085579028459744807e-08, 2.47826837673170067541e-07, 1.02458557534151167038e-05,1.33359012774133036547e-04},
{4.13960387466506916336e-10, 3.00297042360903237935e-11, 1.39609031032134178251e-10, 4.22089681823529186048e-10, 1.51576216533412879788e-09, 4.35183142897237517857e-09, 1.03606921287306709085e-07, 1.36413844453874814870e-06,1.72105085786937265444e-04},
{3.98751503299601609806e-10, 2.63514355907790381676e-11, 1.07686554656210605548e-10, 2.61878911632447840524e-10, 7.21690605921446293735e-10, 1.82713662769675738746e-09, 1.73796814733628284895e-08, 2.48089872785363749994e-07,1.02483613721112087422e-05},
{3.93619910123835259999e-10, 2.43647476830855380912e-11, 8.72364326777568211093e-11, 1.79332714797776976130e-10, 4.48838412493245224553e-10, 6.05086722163088786840e-10, 4.36974519865640910090e-09, 2.92453417943828496973e-08,1.36449596870927225818e-06},
{3.84673139252919370590e-10, 2.26767833361674844057e-11, 7.64998954629607273707e-11, 1.38333498992488097510e-10, 2.78497972272624582533e-10, 2.56315083644442400397e-10, 9.85588262526041625654e-10, 1.73975017290399444799e-08,2.48155667273269683628e-07},
{3.79858159755992507507e-10, 2.14717650157890549015e-11, 7.06467044704199128821e-11, 1.12094946761268893865e-10, 1.90736259990401465264e-10, 1.62238368013736949964e-10, 3.78229205914323906870e-10, 1.95026087598006630036e-09,2.92534385417530507253e-08},
{3.75079045787054076366e-10, 2.10067116476360224507e-11, 6.57447177280917459222e-11, 9.82940342643819691997e-11, 1.47133442160821998071e-10, 9.08959849888468935427e-11, 1.80363085551453197533e-10, 9.86618595112412495737e-10,1.74019592563182137706e-08},
{3.73118630660174161780e-10, 2.03342876744567715017e-11, 6.22559336394143718071e-11, 9.07543012169719241990e-11, 1.19235064175292506318e-10, 6.46175675753521470996e-11, 9.63417184231872534412e-11, 2.34599474393120041450e-10,1.95078800705937746879e-09},
{3.71671622392451197164e-10, 2.00742485119387094162e-11, 6.08624375751502249363e-11, 8.44592859451268964286e-11, 1.04554432254685928024e-10, 5.03748982577806567395e-11, 6.21265662835848666565e-11, 1.26196721830161394373e-10,9.86876327874532589041e-10},
{3.70715155088313717799e-10, 1.97847442783512489471e-11, 5.89302400281014506776e-11, 7.99818668011176630101e-11, 9.65299330279876303190e-11, 4.18337498470057158258e-11, 4.26947571080773169722e-11, 5.67062171333298867168e-11,2.34662115004043134020e-10},
{3.69317757883799583289e-10, 1.96215753104434059326e-11, 5.81600291535647725795e-11, 7.81800619508837943547e-11, 8.98351525936783309992e-11, 3.67025428242264208306e-11, 3.05933210563094981736e-11, 3.47238682286855697434e-11,1.26230004499919781901e-10},
{3.69052553352080403445e-10, 1.95146702083347850456e-11, 5.73241814705526978930e-11, 7.57043432347458629096e-11, 8.50741008929506329381e-11, 3.32785863929529376414e-11, 2.53632131841081127788e-11, 2.43217208563380403540e-11,5.67212655242373402744e-11},
{3.68472849183132989071e-10, 1.94316093358645561333e-11, 5.68481343098615817941e-11, 7.47108748375303775713e-11, 8.31546852777933712550e-11, 3.11428663957611471155e-11, 2.09172299162267348584e-11, 1.66332549062104268888e-11,3.47330697400799117052e-11},
{3.68223423673688893477e-10, 1.93807950874139083945e-11, 5.65350443939310521735e-11, 7.36387402712163889426e-11, 8.05231646392876098759e-11, 2.95907149886755055094e-11, 1.92818786766328049605e-11, 1.41707139168300714739e-11,2.43281553384087609895e-11},
{3.67887153824841764461e-10, 1.93372053710712553550e-11, 5.62930949010441520608e-11, 7.30267025815813273220e-11, 7.94654668421067886869e-11, 2.86425312878749442747e-11, 1.76281543896008841128e-11, 1.18041609317398454443e-11,1.66376730044913355398e-11},
{3.67809400884230354449e-10, 1.93025222561588130311e-11, 5.61439112841143583397e-11, 7.26238347932646472859e-11, 7.83255571540435186990e-11, 2.78808668197523830435e-11, 1.67500742936980328782e-11, 1.06615226510949701773e-11,1.41744691569752790759e-11},
{3.67676359843460851210e-10, 1.92819369207276290556e-11, 5.60172871107575450406e-11, 7.23128731130459296851e-11, 7.76744630966615383765e-11, 2.72788674135835171527e-11, 1.61563287071592359652e-11, 9.87874079435922505299e-12,1.18072929140059931151e-11},
{3.67602524908513794252e-10, 1.92634883229283986465e-11, 5.59170963792486142853e-11, 7.21208074722093525228e-11, 7.72457993596563901788e-11, 2.69330638574918945562e-11, 1.57431210913150406694e-11, 9.38692756804147440490e-12,1.06643503172649172948e-11},
{3.67530108858020060928e-10, 1.92543538907706055811e-11, 5.58571340806560369075e-11, 7.19581636045646575008e-11, 7.69150208403101932816e-11, 2.66184678456137646500e-11, 1.54606208691659274663e-11, 8.90190751383199062497e-12,9.88136047661202633237e-12},
{3.67499120763733505723e-10, 1.92468940936000935118e-11, 5.58039860112146083026e-11, 7.18296310205345732321e-11, 7.67106302189884447471e-11, 2.64697618075621273945e-11, 1.51880655776260398215e-11, 8.71737897140840563144e-12,9.38941622273385161975e-12}, // 35
};

//TODO find a even nicer way to do this
long double sum_binomial(int64_t N, int64_t K, long double p) {
#define MAX_DEPTH 40*9
    static long double *cache[MAX_DEPTH];
    static int64_t max_k[MAX_DEPTH], mid_k[MAX_DEPTH];
    int log2_N = log2(N);
    int log2_P = -log2(p);
    int idx = 9*log2_N + log2_P/2 - 1;

    // avoid large tables and long computation for binomials
    if (log2_N > 35) {
        // Approximate huge binomal distribution by smaller binomial with same mean and variance and p. This is more accurate than using normal distribution
        K = N*p/4 + (K - N*p)/2;
        if (K < 0) {
            return 0;
        } else if (K > N/4)
            return 1;
        return sum_binomial(N/4, K, p);
    }

    assert(N > 0 && K <= N && K >= 0 && p >= 0 && p <= 1);
    assert(log2_N <= 35); // need to build bigger table
    assert(log2_P % 2 == 0 && log2_P >= 2  && log2_P <= 18);
    assert((1ULL << log2_N) == N);
    assert(1.0 / (1ULL << log2_P) == p);

    if (!cache[idx]) {
        int sls4 = ((log2_N&1) ? 1518500249 : 16 << 26) >> (27 - log2_N/2);
        fprintf(stderr, "clean table %d %d\n", log2_N, log2_P); // should only happen once per table
        max_k[idx] = -1;
        mid_k[idx] = (N >> log2_P) + sls4 / (int[]){0,3,5,10,20,40,78,154,304,608}[log2_P/2];
    }

    int64_t k = max_k[idx];
    if (K > k) {
        fprintf(stderr, "fill table %d %d to %"PRId64"\n", log2_N, log2_P, K);
        cache[idx] = realloc(cache[idx], (K+1) * sizeof(**cache));
        long double sum = k >= 0 ? cache[idx][k] : 0;
        for(k++; k <= K; k++) {
            sum += binomial(N, k, p);
            if (k == mid_k[idx])
                sum = -mid_k_table[log2_N][log2_P/2 - 1];
            cache[idx][k] = sum;
        }
        max_k[idx] = k - 1;
    }
    return cache[idx][K] + (K >= mid_k[idx]);
}

static double pbinomial(int64_t N, int64_t K, double p, double correction) {
    if (K < 0)
        return 2.0; // special case for already processed cases
    assert(p == 1.0/4 || p == 1.0/16 || p == 1.0/64 || p == 1.0/256  || p == 1.0/1024 || p == 1.0/4096 || p == 1.0/16384 || p == 1.0/65536 || p == 1.0/262144); // if fails then check/fix next line
    correction /= p;
    //correction is applied due to us testing many patterns and many streams
    long double binp = sum_binomial(N, K, p);
    return fmin(fmin(binp, 1-binp) * correction, 1.0);
}

static int check2(STATET inc1, STATET inc2, STATET seed1, STATET seed2, int index) {
#define LOG2_STATN 18
#define STATN (1<<LOG2_STATN)
#define STAT_HALF 512
// 10,8,6 are the most productive XOR, 16,14.12.10 the most productive interleaved ones for xor, 4 finds a few, 2 even fewer, 12,14,16 are not usefull and could be disabled
#define ALLSTATN (262144 + 65536 + 16384 + 4096 + 1024 + 256 + 64 + 16 + 4)
#define XSTATN (256 + 64 + 16)
    uint8_t prestats[STATN] = {0};
    uint64_t midstats[STATN] = {0};
    int64_t stats[ALLSTATN + XSTATN] = {0};
    static uint8_t stats_basis[ALLSTATN + XSTATN];

    STATET delta = seed1 - seed2;

    for(int64_t i = 0; i< stream_length; i++) {
        OUTT v1, v2;
        v1 = pcg_dxsm(&seed1, multiplier, inc1);
        v2 = pcg_dxsm(&seed2, multiplier, inc2);
#if 0 //XOR only
        int idx = (v1 ^ v2) & (STATN - 1);
#else //capture 8 LSB of both
        int idx = (v1&(STAT_HALF - 1)) + STAT_HALF*(v2&(STAT_HALF - 1));
#endif
        prestats[idx]++;
        if (prestats[idx] >= 255) {
            for(int j = 0; j<STATN; j++) {
                midstats[j] += prestats[j];
                prestats[j] = 0;
            }
        }
    }
    for(int j = 0; j<STATN; j++) {
        static const unsigned interleave[512] = {
            0x00000, 0x00001, 0x00004, 0x00005, 0x00010, 0x00011, 0x00014, 0x00015, 0x00040, 0x00041, 0x00044, 0x00045, 0x00050, 0x00051, 0x00054, 0x00055,
            0x00100, 0x00101, 0x00104, 0x00105, 0x00110, 0x00111, 0x00114, 0x00115, 0x00140, 0x00141, 0x00144, 0x00145, 0x00150, 0x00151, 0x00154, 0x00155,
            0x00400, 0x00401, 0x00404, 0x00405, 0x00410, 0x00411, 0x00414, 0x00415, 0x00440, 0x00441, 0x00444, 0x00445, 0x00450, 0x00451, 0x00454, 0x00455,
            0x00500, 0x00501, 0x00504, 0x00505, 0x00510, 0x00511, 0x00514, 0x00515, 0x00540, 0x00541, 0x00544, 0x00545, 0x00550, 0x00551, 0x00554, 0x00555,
            0x01000, 0x01001, 0x01004, 0x01005, 0x01010, 0x01011, 0x01014, 0x01015, 0x01040, 0x01041, 0x01044, 0x01045, 0x01050, 0x01051, 0x01054, 0x01055,
            0x01100, 0x01101, 0x01104, 0x01105, 0x01110, 0x01111, 0x01114, 0x01115, 0x01140, 0x01141, 0x01144, 0x01145, 0x01150, 0x01151, 0x01154, 0x01155,
            0x01400, 0x01401, 0x01404, 0x01405, 0x01410, 0x01411, 0x01414, 0x01415, 0x01440, 0x01441, 0x01444, 0x01445, 0x01450, 0x01451, 0x01454, 0x01455,
            0x01500, 0x01501, 0x01504, 0x01505, 0x01510, 0x01511, 0x01514, 0x01515, 0x01540, 0x01541, 0x01544, 0x01545, 0x01550, 0x01551, 0x01554, 0x01555,
            0x04000, 0x04001, 0x04004, 0x04005, 0x04010, 0x04011, 0x04014, 0x04015, 0x04040, 0x04041, 0x04044, 0x04045, 0x04050, 0x04051, 0x04054, 0x04055,
            0x04100, 0x04101, 0x04104, 0x04105, 0x04110, 0x04111, 0x04114, 0x04115, 0x04140, 0x04141, 0x04144, 0x04145, 0x04150, 0x04151, 0x04154, 0x04155,
            0x04400, 0x04401, 0x04404, 0x04405, 0x04410, 0x04411, 0x04414, 0x04415, 0x04440, 0x04441, 0x04444, 0x04445, 0x04450, 0x04451, 0x04454, 0x04455,
            0x04500, 0x04501, 0x04504, 0x04505, 0x04510, 0x04511, 0x04514, 0x04515, 0x04540, 0x04541, 0x04544, 0x04545, 0x04550, 0x04551, 0x04554, 0x04555,
            0x05000, 0x05001, 0x05004, 0x05005, 0x05010, 0x05011, 0x05014, 0x05015, 0x05040, 0x05041, 0x05044, 0x05045, 0x05050, 0x05051, 0x05054, 0x05055,
            0x05100, 0x05101, 0x05104, 0x05105, 0x05110, 0x05111, 0x05114, 0x05115, 0x05140, 0x05141, 0x05144, 0x05145, 0x05150, 0x05151, 0x05154, 0x05155,
            0x05400, 0x05401, 0x05404, 0x05405, 0x05410, 0x05411, 0x05414, 0x05415, 0x05440, 0x05441, 0x05444, 0x05445, 0x05450, 0x05451, 0x05454, 0x05455,
            0x05500, 0x05501, 0x05504, 0x05505, 0x05510, 0x05511, 0x05514, 0x05515, 0x05540, 0x05541, 0x05544, 0x05545, 0x05550, 0x05551, 0x05554, 0x05555,
            0x10000, 0x10001, 0x10004, 0x10005, 0x10010, 0x10011, 0x10014, 0x10015, 0x10040, 0x10041, 0x10044, 0x10045, 0x10050, 0x10051, 0x10054, 0x10055,
            0x10100, 0x10101, 0x10104, 0x10105, 0x10110, 0x10111, 0x10114, 0x10115, 0x10140, 0x10141, 0x10144, 0x10145, 0x10150, 0x10151, 0x10154, 0x10155,
            0x10400, 0x10401, 0x10404, 0x10405, 0x10410, 0x10411, 0x10414, 0x10415, 0x10440, 0x10441, 0x10444, 0x10445, 0x10450, 0x10451, 0x10454, 0x10455,
            0x10500, 0x10501, 0x10504, 0x10505, 0x10510, 0x10511, 0x10514, 0x10515, 0x10540, 0x10541, 0x10544, 0x10545, 0x10550, 0x10551, 0x10554, 0x10555,
            0x11000, 0x11001, 0x11004, 0x11005, 0x11010, 0x11011, 0x11014, 0x11015, 0x11040, 0x11041, 0x11044, 0x11045, 0x11050, 0x11051, 0x11054, 0x11055,
            0x11100, 0x11101, 0x11104, 0x11105, 0x11110, 0x11111, 0x11114, 0x11115, 0x11140, 0x11141, 0x11144, 0x11145, 0x11150, 0x11151, 0x11154, 0x11155,
            0x11400, 0x11401, 0x11404, 0x11405, 0x11410, 0x11411, 0x11414, 0x11415, 0x11440, 0x11441, 0x11444, 0x11445, 0x11450, 0x11451, 0x11454, 0x11455,
            0x11500, 0x11501, 0x11504, 0x11505, 0x11510, 0x11511, 0x11514, 0x11515, 0x11540, 0x11541, 0x11544, 0x11545, 0x11550, 0x11551, 0x11554, 0x11555,
            0x14000, 0x14001, 0x14004, 0x14005, 0x14010, 0x14011, 0x14014, 0x14015, 0x14040, 0x14041, 0x14044, 0x14045, 0x14050, 0x14051, 0x14054, 0x14055,
            0x14100, 0x14101, 0x14104, 0x14105, 0x14110, 0x14111, 0x14114, 0x14115, 0x14140, 0x14141, 0x14144, 0x14145, 0x14150, 0x14151, 0x14154, 0x14155,
            0x14400, 0x14401, 0x14404, 0x14405, 0x14410, 0x14411, 0x14414, 0x14415, 0x14440, 0x14441, 0x14444, 0x14445, 0x14450, 0x14451, 0x14454, 0x14455,
            0x14500, 0x14501, 0x14504, 0x14505, 0x14510, 0x14511, 0x14514, 0x14515, 0x14540, 0x14541, 0x14544, 0x14545, 0x14550, 0x14551, 0x14554, 0x14555,
            0x15000, 0x15001, 0x15004, 0x15005, 0x15010, 0x15011, 0x15014, 0x15015, 0x15040, 0x15041, 0x15044, 0x15045, 0x15050, 0x15051, 0x15054, 0x15055,
            0x15100, 0x15101, 0x15104, 0x15105, 0x15110, 0x15111, 0x15114, 0x15115, 0x15140, 0x15141, 0x15144, 0x15145, 0x15150, 0x15151, 0x15154, 0x15155,
            0x15400, 0x15401, 0x15404, 0x15405, 0x15410, 0x15411, 0x15414, 0x15415, 0x15440, 0x15441, 0x15444, 0x15445, 0x15450, 0x15451, 0x15454, 0x15455,
            0x15500, 0x15501, 0x15504, 0x15505, 0x15510, 0x15511, 0x15514, 0x15515, 0x15540, 0x15541, 0x15544, 0x15545, 0x15550, 0x15551, 0x15554, 0x15555
        };

        int idx = interleave[j & (STAT_HALF-1)] + 2*interleave[j >> (LOG2_STATN/2)];
        stats[idx] = midstats[j] + prestats[j];
    }

    int s=0;
    for(int i = LOG2_STATN; i>=2; i-=2){
        int tabsize = 1<<i;
        if (!stats_basis[s])
            memset(stats_basis+s, i, tabsize);
        if (i<LOG2_STATN)
            for (int j = 0; j<(4<<i); j++)
                stats[s + (j&(tabsize-1))] += stats[s - (4<<i) + j];
        s += tabsize;
    }

    //TODO ideally we should also have a 10bit XOR but ATM this doesnt fit too well here
    s  = ALLSTATN;
    int s2 = 262144;
    for(int i = 8; i>=4; i-=2){
        int tabsize = 1<<i;
        for(int j = 0; j<tabsize*tabsize; j++) {
            int j2 = ((j>>1) ^ j) & 0x5555;
            j2 = (j2&1) + ((j2>>1)&2) + ((j2>>2)&4)  + ((j2>>3)&8)
                +((j2>>4)&16) + ((j2>>5)&32) + ((j2>>6)&64) + ((j2>>7)&128);
            stats[s + j2] += stats[s2 + j];
        }
        if (!stats_basis[s])
            memset(stats_basis+s, i, tabsize);
        s += tabsize;
        s2 += tabsize*tabsize*5/4;
    }

    for(int i = 0; i<8; i++) {
        int best = 0;
        double old = pbinomial(stream_length, stats[best], 1.0/(1<<stats_basis[best]), P_CORRECTION);
        for(int j = 1; j<ALLSTATN + XSTATN; j++) {
            double new = pbinomial(stream_length, stats[   j], 1.0/(1<<stats_basis[    j]), P_CORRECTION);
            if (new < old) {
                best = j;
                old = new;
            }
        }
        if (!i) {
            if (old > 0.05) { // we accept 1 in 20 false positives
                return 0;
            }

            printf("%4X: ", index);
            printf(" DELTA: 0x%016"PRIX64" %016"PRIX64" ", (uint64_t)((delta)>>64), (uint64_t)(delta));
        }
        if (old < 0.05){ //less noise, only show significant ones
            printf("%2d%5x(%10"PRId64")%1.9f  ", stats_basis[best] * ((best > ALLSTATN) ? -1 : 1), best & ((1<<stats_basis[best])-1), stats[best], old);
        } else
            break;
        stats[best] = -1;
    }
    printf("\n");
}

static int check(STATET inc1, STATET inc2, STATET seed1, STATET seed2, STATET random_offset) {
    STATET offset = 0;

    //FIXME check if theres anything we can do with mismatching types
    if (inc1 % 4 != inc2 %4)
        return 0;

    STATET offset1 = find_step(seed1, characteristic_seed(inc1), inc1);
    STATET offset2 = find_step(seed2, characteristic_seed(inc2), inc2);
    assert( step(seed1, inc1, offset1) == characteristic_seed(inc1));
    assert( step(seed2, inc2, offset2) == characteristic_seed(inc2));

    seed2 = step(seed2, inc2, offset2 - offset1 + (random_offset & (((STATET)-1) << 128 - topbits)));
    // here seed2 has TOPBITS random and 128 - TOPBITS computet, this is equivalent to consuming 2^(128 - TOPBITS) values from a stream which is hard to do

    printf("checking pair with DELTA: 0x%016"PRIX64" %016"PRIX64" si0: 0x%016"PRIX64" %016"PRIX64",0x%016"PRIX64" %016"PRIX64" si1: 0x%016"PRIX64" %016"PRIX64",0x%016"PRIX64" %016"PRIX64"\n",
           (uint64_t)((seed1 - seed2)>>64), (uint64_t)(seed1 - seed2),
           (uint64_t)((seed1)>>64), (uint64_t)(seed1), (uint64_t)((inc1)>>64), (uint64_t)(inc1),
           (uint64_t)((seed2)>>64), (uint64_t)(seed2), (uint64_t)((inc2)>>64), (uint64_t)(inc2)
          );

    for (int i = LOG2_STATN; i >= 2; i -= 2)
        pbinomial(stream_length, 16*stream_length/(10<<i) + stream_length / 1024, 1.0/(1<<i), P_CORRECTION); // force table to be initialized

    pid_t pidtab[NUM_THREADS];
    pid_t pid = 0;
    for (int i = 0; i<NUM_THREADS; i ++) {
        pid = fork(); //TODO use pthreads, this was just a fun way to speed it up

        if (!pid) {
            int starti, endi;
            starti = i * STREAMS_PER_THREAD;
            endi= starti + STREAMS_PER_THREAD;
            for(int i = starti; i< endi; i++) {
                offset = ((STATET)i) << 128 - topbits;
                STATET moved_seed2 = step(seed2, inc2, offset);
                //TODO skip cases we know will not have correlations
                check2(inc1, inc2, seed1, moved_seed2, i);
            }
            exit(0);
        }
        pidtab[i] = pid;
    }
    for (int i = 0; i<NUM_THREADS; i ++) {
        waitpid(pidtab[i], NULL, 0);
    }
    return 0;
}

int main(int argc, const char **argv) {
    if (argc < 4) {
        fprintf(stderr, "%s <log2 set size> <log2 stream length> <topbits>\n", argv[0]);
        exit(1);
    }
    fprintf(stderr, "Stage 1, filling set with random increments\n");

    int64_t set_size = 1ULL << strtoull(argv[1], NULL, 0);
    stream_length    = 1ULL << strtoull(argv[2], NULL, 0);
    topbits          = strtoull(argv[3], NULL, 0);

    fprintf(stderr, "set size:%"PRId64", stream_length:%"PRId64", top bits:%d\n",
            set_size,
            stream_length,
            topbits);

    STATET seed1, seed2, random_offset;
    STATET *increments = malloc(2*set_size * sizeof(*increments));
    if (!increments) {
        fprintf(stderr, "out of memory\n");
        exit(2);
    }

    FILE *frandom = fopen("/dev/random", "rb");
    if (!frandom) {
        fprintf(stderr, "failed to read /dev/random\n");
        exit(2);
    }
    fread(increments, sizeof(*increments), set_size, frandom);
    for (int64_t i = set_size - 1; i>= 0; i--) {
        increments[2*i] = increments[i] | 1;
        increments[2*i+1] = characteristic_seed(increments[2*i]); //key used for sorting
    }
    fprintf(stderr, "Stage 2, Sorting streams\n");
    qsort(increments, set_size, 2*sizeof(*increments), compar);

    fprintf(stderr, "Stage 3, Comparing streams\n");
    for (int64_t i = 1; i<set_size; i++) {
        int delta = compar(increments + 2*i - 2, increments + 2*i);
        num_pairs_consiered += !delta;
    }
    fprintf(stderr, "pairs to consider: %"PRId64"\n", num_pairs_consiered);

    for (int64_t i = 1; i<set_size; i++) {
        int delta = compar(increments + 2*i - 2, increments + 2*i);

        if (!delta) {
            fread(&seed1, sizeof(seed1), 1, frandom);
            fread(&seed2, sizeof(seed2), 1, frandom); // Not used currently
            fread(&random_offset, sizeof(random_offset), 1, frandom);
            check(increments[2*i - 2], increments[2*i], seed1, seed2, random_offset);
        }
    }
    fclose(frandom);

    return 0;
}
