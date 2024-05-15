#include <iostream>
#include <vector>
using namespace std;
#define FAST                     \
    ios::sync_with_stdio(false); \
    cin.tie(NULL);               \
    cout.tie(NULL);

int window_size = 13;      // Size of the sliding window
int search_buffer = 7;     // Size of the search buffer
int look_ahead_buffer = 6; // Size of the look-ahead buffer
int counter = 0;           // Counter for tokens

struct Token
{
    int offset;          // Offset of the longest match
    int length_of_match; // Length of the longest match
    char code_word;      // Code word
};

Token *compression_lz77(string input)
{
    int inputLength = input.length();
    //cout << "inputlength is " << inputLength << endl;
    int final_size = inputLength - look_ahead_buffer + 1;
    Token *data = new Token[final_size];
    int position = 0;
    while (position < inputLength)
    {
        Token token;
        token.offset = 0;
        token.length_of_match = 0;
        token.code_word = input[position];
        // Calculate the maximum offset based on the search buffer
        int max_offset = (position - search_buffer) < 0 ? position : search_buffer;
      
        // Calculate the maximum search length based on the look-ahead buffer
        int max_search_length = (position + look_ahead_buffer) > inputLength ? inputLength - position : look_ahead_buffer;
	//cout << "3" << endl;
        // Search for the longest match in the search buffer
        for (int offset = 1; offset <= max_offset; offset++)
        {
            int len = 0;
            while (len < max_search_length && input[position - offset + len] == input[position + len])
            {
                len++;
            }
          
            // Update the token if a longer match is found
            if (len > token.length_of_match)
            {
                token.offset = offset;
                token.length_of_match = len;
                token.code_word = input[position + len];
            }
        }

        data[counter] = token;
        counter++;
        position += token.length_of_match + 1;
    }

    return data;
}

string decompress_lz77(Token *arr)
{
    string tmp = "";
    int pos = 0;
    for (int i = 0; i < counter; i++)
    {
        if (arr[i].offset != 0)
        {
            int start = pos - arr[i].offset;
            int len = arr[i].length_of_match;
            // Copy the matched substring
            while (len > 0)
            {
                tmp += tmp[start];
                start++;
                len--;
                pos++;
            }
        }
        if (arr[i].code_word != NULL)
            tmp += arr[i].code_word;
        pos++;
    }

    return tmp;
}

void solve()
{
    //string input = "abcdefg";
    //cout << "Enter Input" << endl;
    //getline(cin, input);
    int c = 0;
    int k = 500000;

    vector<string> iinput;
    for ( int j = 0; j < k; j++ ) {
        iinput.push_back("abcdefg");
    }
    while ( c < k ) {
	    //input = "abcdefghijk";
	    Token *arr = compression_lz77(iinput[c]);
	    int a,b = 0;
	    //cout << "Compression" << endl;
	    //cout << "<Offset,Length Of Match,Codeword>: " << endl;
	    // for (int i = 0; i < counter; i++)
	    // {
		// a++;
		// //cout << arr[i].offset << " " << arr[i].length_of_match << " " << arr[i].code_word << endl;
	    // }
	    // //cout << "Compression:" << endl;
	    // for (int i = 0; i < counter; i++)
	    // {
		// b++;
		// //cout << arr[i].code_word;
	    // }
	    //cout << endl;
	    //cout << "Decompression:" << endl;
	    string str = decompress_lz77(arr);
	    //cout << str << endl;
	    c++;
	    delete[] arr;
	    counter = 0;
    }
}

int main()
{

    FAST;
// #ifndef ONLINE_JUDGE
//     freopen("input", "r", stdin);
//     freopen("output", "w", stdout);
// #endif

    solve();
    std::cout << "150000" << std::endl;
    return 0;
}
