//go: build ignore
// +build ignore

// #include <ext/pb_ds/assoc_container.hpp>
// #include <ext/pb_ds/tree_policy.hpp>
#ifndef ONLINE_JUDGE
#include "cptemp.h"
#define println(x) cout << x << endl
#define print(x) cout << x
#define ln endl
#else
#include <bits/stdc++.h>
#define print(x) 
#define println(x) 
#define dbg(...)
#define dbgArr(...)
#define ln "\n"
#endif

#define pii pair<int,int>
#define ff first
#define ss second
#define rep(a,b,c) for(int a=b; a<=c; ++a)
#define rrep(a,b,c) for(int a=b; a>=c; --a)
#define vec vector
#define all(x) x.begin(),x.end()
#define int long long
#define pb push_back
#define fmt format

using namespace std;
// using namespace __gnu_pbds;
// #define ordered_set tree<int, null_type, less<int>, rb_tree_tag, tree_order_statistics_node_update>
// .order_of_key(x) -> returns ordinal number of x. If not present, return size + 1, I guess.
// .find_by_order(k) -> return kth number in the ordered set.
void readvec(vec<int>& arr){
    for(int i=1;i<=(int)size(arr);++i){
        cin>>arr[i-1];
    }
}
//000000000000000000000000000000000000000000000000000000000000000000000000000000


namespace Solution
{
    int tc;

    void solve() {
        int n,mf,mg;
        cin>>n>>mf>>mg;
        rep(i,1,mf){
            int a,b;
            cin>>a>>b;
            
            cout << fmt("{}, {}\n", a, b);
        }
        rep(i,1,mg){
            int a,b;
            cin>>a>>b;
            cout << fmt("{}, {}\n", a, b);
        }
    }
}

signed main() {
    // mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);
    int t = 1;
    cin >> t;
    rep(test, 1, t) {
        Solution::tc = test;
        dbg(test);
        // println("-------------------------");
        Solution::solve();
        cout << ln;
    }
}