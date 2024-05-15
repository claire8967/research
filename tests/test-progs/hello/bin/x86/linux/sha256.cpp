/**
 * SHA-256 implemented according to the specification:
 * http://csrc.nist.gov/publications/fips/fips180-4/fips-180-4.pdf
 */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include <map>
#include <cassert>

typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long long ll;

// Constants used in hash algorithm
const WORD K[] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::vector<std::vector<WORD>> M; // Message to be hashed
std::vector<std::vector<WORD>> H; // Hashed message
std::vector<BYTE> bytes;          // Plain and padded message bytes
WORD W[64];                       // Message schedule
ll l = 0;                         // Message length in bits
int N;                            // Number of blocks in padded message

// Working variables
WORD a, b, c, d, e, f, g, h;

// Temporary words
WORD T1, T2;


/**
 * Take the given hexadecimal string and store the bytes in a global vector.
 * Also update the message length.
 */
const void store_message_bytes(const std::string &hex_str)
{
    int str_length = hex_str.length();
    for (int i = 0, j = 0; i < str_length / 2; ++i, j += 2)
    {
        BYTE byte = std::stoi(hex_str.substr(j, 2), nullptr, 16);
        bytes.push_back(byte);
        l += 8;
    }
}

/**
 * Calculate the required padding of the message.
 * Return the required padding.
 */
const int calc_padding()
{
    int k = 0;
    while ((l + 1 + k) % 512 != 448) ++k;
    return k;
}

/**
 * Pad the message bytes according to the specification.
 */
const void pad_message()
{
    int k = calc_padding();
    // Append 1 followed by zeroes before the least significant bit,
    // assuming full bytes as input
    bytes.push_back(0x80);
    k = k - 7;

    // Append the remaining zeroes of k
    for (int i = 0; i < (k / 8); ++i)
    {
        bytes.push_back(0);
    }

    // Finally append the length in binary to the message as the least
    // significant bits, assuming a 64-bit number
    for (int i = 1; i < 9; ++i)
    {
        bytes.push_back(l >> (64 - i * 8));
    }
}

/**
 * Parse the message into N 512-bit blocks split up into words.
 * Also update N.
 */
const void parse_message()
{
    WORD n = 0;
    for (int i = 0; n < bytes.size() / 64; ++n)
    {
        std::vector<WORD> block(16);
        for (int j = 0; j < 16; ++j)
        {
            WORD word = 0;
            for (int k = 0; k < 4; ++k, ++i)
            {
                word <<= 8;
                word |= bytes[i];
            }
            block[j] = word;
        }
        M.push_back(block);
    }
    N = n;
}

/**
 * Initialise the hash value H_0.
 */
const void init_hash()
{
    std::vector<WORD> h0 = {0x6a09e667,
                            0xbb67ae85,
                            0x3c6ef372,
                            0xa54ff53a,
                            0x510e527f,
                            0x9b05688c,
                            0x1f83d9ab,
                            0x5be0cd19};
    H.push_back(h0);
}

/**
 * Rotate right function ROTR^n(x) in hash algorithm.
 */
const WORD ROTR(const WORD &n, const WORD &x)
{
    return (x >> n) | (x << (32 - n));
}

/**
 * Right shift function SHR^n(x) in hash algorithm.
 */
const WORD SHR(const WORD &n, const WORD &x)
{
    return x >> n;
}

/**
 * Logical function Ch(x, y, z) in hash algorithm.
 */
const WORD Ch(const WORD &x, const WORD &y, const WORD &z)
{
    return (x & y) ^ (~x & z);
}

/**
 * Logical function Maj(x, y, z) in hash algorithm.
 */
const WORD Maj(const WORD &x, const WORD &y, const WORD &z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

/**
 * Logical function (large) sigma^256_0(x) in hash algorithm.
 */
const WORD lsigma0(const WORD &x)
{
    return ROTR(2, x) ^ ROTR(13, x) ^ ROTR(22, x);
}

/**
 * Logical function (large) sigma^256_1(x) in hash algorithm.
 */
const WORD lsigma1(const WORD &x)
{
    return ROTR(6, x) ^ ROTR(11, x) ^ ROTR(25, x);
}

/**
 * Logical function (small) sigma^256_0(x) in hash algorithm.
 */
const WORD ssigma0(const WORD &x)
{
    return ROTR(7, x) ^ ROTR(18, x) ^ SHR(3, x);
}

/**
 * Logical function (small) sigma^256_1(x) in hash algorithm.
 */
const WORD ssigma1(const WORD &x)
{
    return ROTR(17, x) ^ ROTR(19, x) ^ SHR(10, x);
}

/**
 * Compute the hash value.
 */
const void compute_hash()
{
    std::vector<WORD> hash_block(8);
    for (int i = 1; i <= N; ++i)
    {
        // Prepare message schedule
        for (int t = 0; t <= 15; ++t)
            W[t] = M[i - 1][t]; // M^i in spec
        for (int t = 16; t <= 63; ++t)
            W[t] = ssigma1(W[t - 2]) + W[t - 7] + ssigma0(W[t - 15]) + W[t - 16];

        // Initialise working variables with previous hash value
        a = H[i - 1][0];
        b = H[i - 1][1];
        c = H[i - 1][2];
        d = H[i - 1][3];
        e = H[i - 1][4];
        f = H[i - 1][5];
        g = H[i - 1][6];
        h = H[i - 1][7];

        // Perform logical operations
        for (int t = 0; t <= 63; ++t)
        {
            T1 = h + lsigma1(e) + Ch(e, f, g) + K[t] + W[t];
            T2 = lsigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        // Compute intermediate hash values by assigning them to H^i
        hash_block[0] = a + H[i - 1][0];
        hash_block[1] = b + H[i - 1][1];
        hash_block[2] = c + H[i - 1][2];
        hash_block[3] = d + H[i - 1][3];
        hash_block[4] = e + H[i - 1][4];
        hash_block[5] = f + H[i - 1][5];
        hash_block[6] = g + H[i - 1][6];
        hash_block[7] = h + H[i - 1][7];
        H.push_back(hash_block);
    }
}

/**
 * Output the generated hash value as a hexadecimal string.
 */
const void output_hash()
{
    // Concatenate the final hash blocks
    for (int i = 0; i < 8; ++i)
        std::cout << std::hex << std::setw(8) << std::setfill('0') << H[N][i];
    std::cout << std::endl;
}

/**
 * Clear all working vectors and variables.
 */
const void clear()
{
    M.clear();
    H.clear();
    bytes.clear();
    l = 0;
}

std::string random_string(std::size_t length)
{
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

int main()
{
    //1000 -> 40 %
    std::vector<std::string> line;
    for ( int i = 0; i < 600; i++ ) {
        //line.push_back(random_string(20));
        line.push_back("11223344555577889911223344555577889911223344555577889911223344555577889911223344555577889911223344555577812121212494949494121212");
        line.push_back("");
        line.push_back("3bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284e62fcdfae01a5ef57d16d645b6e6c24f69b36e12a11f5829acc7ea5082dbd69de25a7b2ba78aafe6ec37036e7f33537a31935627a29a9011866275a6b1eedfb3036dc91104b374617f565d1a6b9c9abab9e2e062fc061f37c8bb8c7138ffae3097ef5c0d1754fd4b13eac8e805a31c8c6657554ed2c60054361e7c6180ec17f4b6b439fe626ec4bf79a02885af9318b79");
        line.push_back("de188941a3375d3a8a061e67576e926d");
        line.push_back("e5");
        line.push_back("3bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284003bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284003bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284003bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284003bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284013bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976e284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284003bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284bef0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284a1f0e8c1157b2be9a4c8e57bebb3ab44be2466084976b325a5953dd7418ee284013bf0e8c1157b2be9a4c8e57bebb3ab44be2466084976a2a2a2a2a2a2a2a2a2a280ffefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefefee939393939393939393939392101010");
        line.push_back("11223344555577889911223344555577889911223344555577889911223344555577889911223344555577889911223344555577812121212494949494121212129685746375869786abe4a6f30200");
        line.push_back("4719");
        line.push_back("93006a");
        line.push_back("889468b1");
        line.push_back("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        line.push_back("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        line.push_back("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
        line.push_back("88b4b6f3b3c7e6a8519477a71c605fcfcdf636718d9bb55363cbd15a0eac58ca");
        line.push_back("5d52a73def39b2e4eb98725300b36c7f26100152b25633301e61cfd2e3c129ce");
        line.push_back("02eaeaeba71b64a97cc41c83625e497e64d991e0966773131b143689e50bd87d");
    }

    for( int i = 0; i < 1; i++) {
    // Read each line as a hexadecimal string to be hashed
        for ( int j = 0; j < line.size(); j++ ) {
            // Store the plain bytes of the message
            
            store_message_bytes(line[j]);

            // Pad the bytes (if necessary)
            pad_message();

            // Parse message into word blocks
            parse_message();

            // Set the inital hash value
            init_hash();

            // Compute the hash value
            compute_hash();

            // Output the generated hash value
            // if( i == 0 ) {
            //     output_hash();
            // }
            // Reset message to hash a new one
            clear();
        }
    }
    std::cout << "600" <<std::endl;
}
