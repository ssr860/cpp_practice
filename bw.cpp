

#include <iostream>
#include <vector>
#include <string>

using namespace std;

bool helper(int n, const vector<string> &v, int start) {
    int pos1 = -1, sx1 = 0;

    for (int i = start; i < n; i++) {
        if ((v[0][i] == 'B' && v[1][i] == 'B') || (v[0][i] == 'W' && v[1][i] == 'W')) continue; 
        
        pos1 = i;
        sx1 = (v[0][i] == 'B' ? 0 : 1);
        break;
    }

    if (pos1 == -1) return true;

    int pos2 = -1, sx2 = 0;

    for (int i = pos1 + 1; i < n; i++) {
        if (v[0][i] == 'W' && v[1][i] == 'W') return false; 
        if (v[0][i] == 'B' && v[1][i] == 'B') return false; 
        
        pos2 = i;
        sx2 = (v[0][i] == 'B' ? 0 : 1);
        break;
    }

    if (pos2 == -1) return false; 

    int dist = pos2 - pos1;

    if ((sx1 == sx2 && dist % 2 != 0) || (sx1 != sx2 && dist % 2 == 0)) {
        return helper(n, v, pos2 + 1);
    }
    return false;
}

int main(){
    int n;
    cin >> n;
    vector<string> v(2);
    cin >> v[0] >> v[1];
    
    if (helper(n,v,0)) cout << "YES" << endl;
    else cout << "NO" << endl;
    
    return 0;
}
