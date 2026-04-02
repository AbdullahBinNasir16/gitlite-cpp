/*
GitLite
Data Structures - Semester Project
Group Members:
Abubakar       23i-0515 A
Abbas Razza    23i-0803 A
Abdullah bin Nasir  23i-0733 A
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <iomanip>
#include <climits>
#ifdef _WIN32
#include <direct.h>
#define DO_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define DO_MKDIR(p) mkdir(p, 0755)
#endif
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

string makePath(const string &folder, const string &file)
{
    return folder + "/" + file + ".node";
}

string makeCommitLogPath(const string &folder)
{
    return folder + "/COMMITS.log";
}

bool fileExists(const string &path)
{
    ifstream f(path);
    return f.good();
}

void createDir(const string &path)
{
    DO_MKDIR(path.c_str());
}

void copyDirRecursive(const string &src, const string &dst)
{
    createDir(dst);
    try
    {
        fs::copy(src, dst,
                 fs::copy_options::recursive |
                     fs::copy_options::overwrite_existing);
    }
    catch (...)
    {
    }
}

string getCurrentTimestamp()
{
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return string(buf);
}

string trim(const string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == string::npos) ? "" : s.substr(a, b - a + 1);
}

// CSV helper ftns

vector<string> splitCSV(const string &line)
{
    vector<string> cols;
    string cur;
    bool inQ = false;
    for (char c : line)
    {
        if (c == '"')
            inQ = !inQ;
        else if (c == ',' && !inQ)
        {
            cols.push_back(trim(cur));
            cur = "";
        }
        else
            cur += c;
    }
    cols.push_back(trim(cur));
    return cols;
}

// col Inde starts from 1
string getCol(const string &row, int colIndex)
{
    auto cols = splitCSV(row);
    if (colIndex < 1 || colIndex > (int)cols.size())
        return "";
    return cols[colIndex - 1];
}

// HASHING

// Instructor hash: multiply digits or ascii value
long long instructorHash(const string &val)
{
    bool isNum = !val.empty();
    for (char c : val)
        if (!isdigit(c) && c != '.' && c != '-')
        {
            isNum = false;
            break;
        }

    long long r = 1;
    if (isNum)
    {
        for (char c : val)
        {
            int d = c - '0';
            if (isdigit(c) && d != 0)
                r *= d;
        }
    }
    else
    {
        for (unsigned char c : val)
        {
            r *= (long long)c;
            if (r > (long long)1e15)
                r %= 999983;
        }
    }
    return ((r % 29) + 29) % 29;
}

// merkle tree hash
string hashData(const string &data)
{
    unsigned long long h1 = 0xcbf29ce484222325ULL;
    unsigned long long h2 = 0x9368e9b5a4e87efdULL;
    unsigned long long h3 = 0x352cf29b4a9a7a59ULL;
    unsigned long long h4 = 0x14a5b3d2c7e8f901ULL;
    for (size_t i = 0; i < data.size(); i++)
    {
        unsigned char c = (unsigned char)data[i];
        h1 ^= c;
        h1 *= 0x00000100000001B3ULL;
        h2 ^= (c + i);
        h2 *= 0x00000100000001B3ULL;
        h3 ^= (c * 31 + i * 7);
        h3 *= 0x00000100000001B3ULL;
        h4 ^= (c ^ (i << 2));
        h4 *= 0x00000100000001B3ULL;
    }
    ostringstream oss;
    oss << hex << setfill('0')
        << setw(16) << h1 << setw(16) << h2
        << setw(16) << h3 << setw(16) << h4;
    return oss.str();
}

// Node File struct for AVL n RB trees
// Line 0 : CSV row data
// Line 1 : left child filename
// Line 2 : right child filename
// Line 3 : parent filename
// Line 4 : height for AVL / color "R"or"B" for RB tree
// Line 5 : merkle hash

struct NodeFile
{
    string data;
    string left = "NULL";
    string right = "NULL";
    string parent = "ROOT";
    int height = 1;
    string color = "R";
    string merkle = "";
};

void readNode(const string &path, NodeFile &n)
{
    ifstream f(path);
    string h;
    getline(f, n.data);
    getline(f, n.left);
    getline(f, n.right);
    getline(f, n.parent);
    getline(f, h);
    n.height = h.empty() ? 1 : stoi(h);
    getline(f, n.color);
    getline(f, n.merkle);
}

void writeNode(const string &path, const NodeFile &n)
{
    ofstream f(path);
    f << n.data << "\n"
      << n.left << "\n"
      << n.right << "\n"
      << n.parent << "\n"
      << n.height << "\n"
      << n.color << "\n"
      << n.merkle << "\n";
}

// makw a filename from a column value + row number
string makeFilename(const string &val, int row)
{
    string s = val;
    for (char &c : s)
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    if (s.size() > 30)
        s = s.substr(0, 30);
    return s + "_" + to_string(row);
}

// AVL TREE

class AVLTree
{
    string folder;
    string root;
    int col;

    // helper ftns
    string path(const string &f) { return makePath(folder, f); }

    int height(const string &f)
    {
        if (f == "NULL")
            return 0;
        if (!fileExists(path(f)))
            return 0;
        NodeFile n;
        readNode(path(f), n);
        return n.height;
    }

    int bf(const string &f)
    {
        if (f == "NULL")
            return 0;
        NodeFile n;
        readNode(path(f), n);
        return height(n.left) - height(n.right);
    }

    void updateH(const string &f)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readNode(path(f), n);
        n.height = max(height(n.left), height(n.right)) + 1;
        writeNode(path(f), n);
    }

    long long key(const string &f)
    {
        if (f == "NULL")
            return LLONG_MIN;
        NodeFile n;
        readNode(path(f), n);
        return instructorHash(getCol(n.data, col));
    }

    // rotations
    string rotR(const string &y)
    {
        NodeFile Y, X;
        readNode(path(y), Y);
        string x = Y.left;
        readNode(path(x), X);
        string T2 = X.right;

        X.right = y;
        X.parent = Y.parent;
        Y.left = T2;
        Y.parent = x;
        if (T2 != "NULL")
        {
            NodeFile t;
            readNode(path(T2), t);
            t.parent = y;
            writeNode(path(T2), t);
        }
        writeNode(path(y), Y);
        writeNode(path(x), X);
        updateH(y);
        updateH(x);
        return x;
    }

    string rotL(const string &x)
    {
        NodeFile X, Y;
        readNode(path(x), X);
        string y = X.right;
        readNode(path(y), Y);
        string T2 = Y.left;

        Y.left = x;
        Y.parent = X.parent;
        X.right = T2;
        X.parent = y;
        if (T2 != "NULL")
        {
            NodeFile t;
            readNode(path(T2), t);
            t.parent = x;
            writeNode(path(T2), t);
        }
        writeNode(path(x), X);
        writeNode(path(y), Y);
        updateH(x);
        updateH(y);
        return y;
    }

    // recursive insert
    string ins(const string &cur, const string &fname, const string &data)
    {
        if (cur == "NULL")
        {
            NodeFile n;
            n.data = data;
            n.height = 1;
            writeNode(path(fname), n);
            return fname;
        }
        NodeFile c;
        readNode(path(cur), c);
        long long nk = instructorHash(getCol(data, col));
        long long ck = instructorHash(getCol(c.data, col));

        if (nk < ck)
        {
            c.left = ins(c.left, fname, data);
            NodeFile lc;
            readNode(path(c.left), lc);
            lc.parent = cur;
            writeNode(path(c.left), lc);
        }
        else
        {
            c.right = ins(c.right, fname, data);
            NodeFile rc;
            readNode(path(c.right), rc);
            rc.parent = cur;
            writeNode(path(c.right), rc);
        }
        writeNode(path(cur), c);
        updateH(cur);

        readNode(path(cur), c);
        int b = bf(cur);
        if (b > 1 && nk < key(c.left))
            return rotR(cur);
        if (b < -1 && nk >= key(c.right))
            return rotL(cur);
        if (b > 1 && nk >= key(c.left))
        {
            c.left = rotL(c.left);
            writeNode(path(cur), c);
            return rotR(cur);
        }
        if (b < -1 && nk < key(c.right))
        {
            c.right = rotR(c.right);
            writeNode(path(cur), c);
            return rotL(cur);
        }
        return cur;
    }

    string minNode(const string &f)
    {
        NodeFile n;
        readNode(path(f), n);
        return (n.left == "NULL") ? f : minNode(n.left);
    }

    string del(const string &cur, long long k)
    {
        if (cur == "NULL")
            return "NULL";
        NodeFile c;
        readNode(path(cur), c);
        long long ck = instructorHash(getCol(c.data, col));

        if (k < ck)
        {
            c.left = del(c.left, k);
            writeNode(path(cur), c);
        }
        else if (k > ck)
        {
            c.right = del(c.right, k);
            writeNode(path(cur), c);
        }
        else
        {
            if (c.left == "NULL" || c.right == "NULL")
            {
                string child = (c.left != "NULL") ? c.left : c.right;
                remove(path(cur).c_str());
                if (child != "NULL")
                {
                    NodeFile ch;
                    readNode(path(child), ch);
                    ch.parent = c.parent;
                    writeNode(path(child), ch);
                }
                return child;
            }
            string succ = minNode(c.right);
            NodeFile sn;
            readNode(path(succ), sn);
            c.data = sn.data;
            c.right = del(c.right, instructorHash(getCol(sn.data, col)));
            writeNode(path(cur), c);
        }
        updateH(cur);
        readNode(path(cur), c);
        int b = bf(cur);
        if (b > 1 && bf(c.left) >= 0)
            return rotR(cur);
        if (b > 1 && bf(c.left) < 0)
        {
            c.left = rotL(c.left);
            writeNode(path(cur), c);
            return rotR(cur);
        }
        if (b < -1 && bf(c.right) <= 0)
            return rotL(cur);
        if (b < -1 && bf(c.right) > 0)
        {
            c.right = rotR(c.right);
            writeNode(path(cur), c);
            return rotL(cur);
        }
        return cur;
    }

    void inorder(const string &f, int depth)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readNode(path(f), n);
        inorder(n.left, depth + 1);
        cout << string(depth * 2, ' ') << getCol(n.data, col) << "  [h=" << n.height << "]\n";
        inorder(n.right, depth + 1);
    }

    void viz(const string &f, const string &pre, bool left)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readNode(path(f), n);
        cout << pre << (left ? "+-- " : "\\-- ") << getCol(n.data, col) << "\n";
        string np = pre + (left ? "|   " : "    ");
        viz(n.left, np, true);
        viz(n.right, np, false);
    }

    string merkle(const string &f)
    {
        if (f == "NULL")
            return string(16, '0');
        NodeFile n;
        readNode(path(f), n);
        string lh = merkle(n.left), rh = merkle(n.right);
        string h = hashData(lh + n.data + rh);
        n.merkle = h;
        writeNode(path(f), n);
        return h;
    }

public:
    void init(const string &dir, int c)
    {
        folder = dir;
        root = "NULL";
        col = c;
        createDir(folder);
    }
    void setFolder(const string &d) { folder = d; }
    void setCol(int c) { col = c; }
    void setRoot(const string &r) { root = r; }
    string getRoot() { return root; }

    void insert(const string &data, int row)
    {
        string fname = makeFilename(getCol(data, col), row);
        root = ins(root, fname, data);
        if (root != "NULL")
        {
            NodeFile n;
            readNode(path(root), n);
            n.parent = "ROOT";
            writeNode(path(root), n);
        }
    }

    void remove(const string &keyVal)
    {
        root = del(root, instructorHash(keyVal));
        if (root != "NULL")
        {
            NodeFile n;
            readNode(path(root), n);
            n.parent = "ROOT";
            writeNode(path(root), n);
        }
    }

    bool search(const string &keyVal, string &out)
    {
        long long k = instructorHash(keyVal);
        string cur = root;
        while (cur != "NULL")
        {
            NodeFile n;
            readNode(path(cur), n);
            long long ck = instructorHash(getCol(n.data, col));
            if (k == ck)
            {
                out = n.data;
                return true;
            }
            cur = (k < ck) ? n.left : n.right;
        }
        return false;
    }

    void printInorder()
    {
        if (root == "NULL")
        {
            cout << "  (empty)\n";
            return;
        }
        inorder(root, 0);
    }
    void visualize()
    {
        if (root == "NULL")
        {
            cout << "  (empty)\n";
            return;
        }
        NodeFile n;
        readNode(path(root), n);
        cout << getCol(n.data, col) << "\n";
        viz(n.left, "", true);
        viz(n.right, "", false);
    }
    string merkleRoot() { return merkle(root); }
};

// RED BLACK TREE

class RBTree
{
    string folder;
    string root;
    int col;

    string path(const string &f) { return makePath(folder, f); }

    void readRB(const string &f, NodeFile &n) { readNode(path(f), n); }
    void writeRB(const string &f, const NodeFile &n) { writeNode(path(f), n); }

    bool isRed(const string &f)
    {
        if (f == "NULL")
            return false;
        NodeFile n;
        readRB(f, n);
        return n.color == "R";
    }

    void setColor(const string &f, const string &c)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readRB(f, n);
        n.color = c;
        writeRB(f, n);
    }

    long long key(const string &f)
    {
        if (f == "NULL")
            return LLONG_MIN;
        NodeFile n;
        readRB(f, n);
        return instructorHash(getCol(n.data, col));
    }

    string rotL(const string &x, const string &xp)
    {
        NodeFile X, Y;
        readRB(x, X);
        string y = X.right;
        readRB(y, Y);
        X.right = Y.left;
        if (Y.left != "NULL")
        {
            NodeFile t;
            readRB(Y.left, t);
            t.parent = x;
            writeRB(Y.left, t);
        }
        Y.parent = xp;
        X.parent = y;
        Y.left = x;
        writeRB(x, X);
        writeRB(y, Y);
        if (xp != "ROOT")
        {
            NodeFile p;
            readRB(xp, p);
            if (p.left == x)
                p.left = y;
            else
                p.right = y;
            writeRB(xp, p);
        }
        else
            root = y;
        return y;
    }

    string rotR(const string &y, const string &yp)
    {
        NodeFile Y, X;
        readRB(y, Y);
        string x = Y.left;
        readRB(x, X);
        Y.left = X.right;
        if (X.right != "NULL")
        {
            NodeFile t;
            readRB(X.right, t);
            t.parent = y;
            writeRB(X.right, t);
        }
        X.parent = yp;
        Y.parent = x;
        X.right = y;
        writeRB(y, Y);
        writeRB(x, X);
        if (yp != "ROOT")
        {
            NodeFile p;
            readRB(yp, p);
            if (p.left == y)
                p.left = x;
            else
                p.right = x;
            writeRB(yp, p);
        }
        else
            root = x;
        return x;
    }

    void fixInsert(string z)
    {
        while (true)
        {
            NodeFile Z;
            readRB(z, Z);
            if (Z.parent == "ROOT" || !isRed(Z.parent))
                break;
            string par = Z.parent;
            NodeFile P;
            readRB(par, P);
            string gp = P.parent;
            if (gp == "ROOT")
                break;
            NodeFile GP;
            readRB(gp, GP);

            if (GP.left == par)
            {
                string unc = GP.right;
                if (isRed(unc))
                {
                    setColor(par, "B");
                    setColor(unc, "B");
                    setColor(gp, "R");
                    z = gp;
                }
                else
                {
                    if (P.right == z)
                    {
                        z = par;
                        rotL(z, gp);
                        readRB(z, Z);
                        par = Z.parent;
                        readRB(par, P);
                        gp = P.parent;
                    }
                    setColor(par, "B");
                    setColor(gp, "R");
                    rotR(gp, GP.parent);
                }
            }
            else
            {
                string unc = GP.left;
                if (isRed(unc))
                {
                    setColor(par, "B");
                    setColor(unc, "B");
                    setColor(gp, "R");
                    z = gp;
                }
                else
                {
                    if (P.left == z)
                    {
                        z = par;
                        rotR(z, gp);
                        readRB(z, Z);
                        par = Z.parent;
                        readRB(par, P);
                        gp = P.parent;
                    }
                    setColor(par, "B");
                    setColor(gp, "R");
                    rotL(gp, GP.parent);
                }
            }
        }
        setColor(root, "B");
    }

    void inorder(const string &f, int depth)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readRB(f, n);
        inorder(n.left, depth + 1);
        cout << string(depth * 2, ' ') << getCol(n.data, col) << " (" << n.color << ")\n";
        inorder(n.right, depth + 1);
    }

    void viz(const string &f, const string &pre, bool left)
    {
        if (f == "NULL")
            return;
        NodeFile n;
        readRB(f, n);
        cout << pre << (left ? "+-- " : "\\-- ")
             << getCol(n.data, col) << "(" << n.color << ")\n";
        string np = pre + (left ? "|   " : "    ");
        viz(n.left, np, true);
        viz(n.right, np, false);
    }

    string merkle(const string &f)
    {
        if (f == "NULL")
            return string(16, '0');
        NodeFile n;
        readRB(f, n);
        string h = hashData(merkle(n.left) + n.data + merkle(n.right));
        n.merkle = h;
        writeRB(f, n);
        return h;
    }

public:
    void init(const string &dir, int c)
    {
        folder = dir;
        root = "NULL";
        col = c;
        createDir(folder);
    }
    void setFolder(const string &d) { folder = d; }
    void setCol(int c) { col = c; }
    void setRoot(const string &r) { root = r; }
    string getRoot() { return root; }

    void insert(const string &data, int row)
    {
        string fname = makeFilename(getCol(data, col), row);
        long long nk = instructorHash(getCol(data, col));

        NodeFile nn;
        nn.data = data;
        nn.color = "R";

        if (root == "NULL")
        {
            nn.color = "B";
            writeRB(fname, nn);
            root = fname;
            return;
        }

        string cur = root, par = "ROOT";
        while (cur != "NULL")
        {
            par = cur;
            NodeFile c;
            readRB(cur, c);
            cur = (nk < key(cur)) ? c.left : c.right;
        }
        nn.parent = par;
        writeRB(fname, nn);
        NodeFile P;
        readRB(par, P);
        if (nk < key(par))
            P.left = fname;
        else
            P.right = fname;
        writeRB(par, P);
        fixInsert(fname);
    }

    bool search(const string &keyVal, string &out)
    {
        long long k = instructorHash(keyVal);
        string cur = root;
        while (cur != "NULL")
        {
            NodeFile n;
            readRB(cur, n);
            long long ck = instructorHash(getCol(n.data, col));
            if (k == ck)
            {
                out = n.data;
                return true;
            }
            cur = (k < ck) ? n.left : n.right;
        }
        return false;
    }

    void printInorder()
    {
        if (root == "NULL")
        {
            cout << "  (empty)\n";
            return;
        }
        inorder(root, 0);
    }
    void visualize()
    {
        if (root == "NULL")
        {
            cout << "  (empty)\n";
            return;
        }
        NodeFile n;
        readRB(root, n);
        cout << getCol(n.data, col) << "(B)\n";
        viz(n.left, "", true);
        viz(n.right, "", false);
    }
    string merkleRoot() { return merkle(root); }
};


// B-TREE 
// Node file format:
// Line 0 : numKeys
// Lines 1..n : key row_data
// Line n+1 : isLeaf 
// Lines n+2.. : child filenames 

struct BNode
{
    int numKeys = 0;
    vector<long long> keys;
    vector<string> data;
    bool isLeaf = true;
    vector<string> children;
};

class BTree
{
    string folder;
    string root;
    int t; // minimum degree
    int col;
    int cnt = 0; // node counter for unique filenames

    string path(const string &f) { return makePath(folder, f); }
    string newFile() { return "b_" + to_string(++cnt); }

    void readB(const string &f, BNode &n)
    {
        ifstream fi(path(f));
        string line;
        getline(fi, line);
        n.numKeys = stoi(line);
        n.keys.resize(n.numKeys);
        n.data.resize(n.numKeys);
        for (int i = 0; i < n.numKeys; i++)
        {
            getline(fi, line);
            size_t tab = line.find('\t');
            n.keys[i] = stoll(line.substr(0, tab));
            n.data[i] = line.substr(tab + 1);
        }
        getline(fi, line);
        n.isLeaf = (line == "1");
        if (!n.isLeaf)
        {
            n.children.resize(n.numKeys + 1);
            for (int i = 0; i <= n.numKeys; i++)
                getline(fi, n.children[i]);
        }
    }

    void writeB(const string &f, const BNode &n)
    {
        ofstream fi(path(f));
        fi << n.numKeys << "\n";
        for (int i = 0; i < n.numKeys; i++)
            fi << n.keys[i] << "\t" << n.data[i] << "\n";
        fi << (n.isLeaf ? "1" : "0") << "\n";
        if (!n.isLeaf)
            for (int i = 0; i <= n.numKeys; i++)
                fi << n.children[i] << "\n";
    }

    void splitChild(const string &par, int idx, const string &child)
    {
        BNode P, C;
        readB(par, P);
        readB(child, C);
        BNode R;
        R.isLeaf = C.isLeaf;
        R.numKeys = t - 1;
        string rf = newFile();
        for (int j = 0; j < t - 1; j++)
        {
            R.keys.push_back(C.keys[j + t]);
            R.data.push_back(C.data[j + t]);
        }
        if (!C.isLeaf)
            for (int j = 0; j < t; j++)
                R.children.push_back(C.children[j + t]);
        P.children.insert(P.children.begin() + idx + 1, rf);
        P.keys.insert(P.keys.begin() + idx, C.keys[t - 1]);
        P.data.insert(P.data.begin() + idx, C.data[t - 1]);
        P.numKeys++;
        C.keys.resize(t - 1);
        C.data.resize(t - 1);
        C.numKeys = t - 1;
        if (!C.isLeaf)
            C.children.resize(t);
        writeB(child, C);
        writeB(rf, R);
        writeB(par, P);
    }

    void insNonFull(const string &f, long long k, const string &data)
    {
        BNode n;
        readB(f, n);
        int i = n.numKeys - 1;
        if (n.isLeaf)
        {
            n.keys.push_back(0);
            n.data.push_back("");
            while (i >= 0 && k < n.keys[i])
            {
                n.keys[i + 1] = n.keys[i];
                n.data[i + 1] = n.data[i];
                i--;
            }
            n.keys[i + 1] = k;
            n.data[i + 1] = data;
            n.numKeys++;
            writeB(f, n);
        }
        else
        {
            while (i >= 0 && k < n.keys[i])
                i--;
            i++;
            BNode ch;
            readB(n.children[i], ch);
            if (ch.numKeys == 2 * t - 1)
            {
                splitChild(f, i, n.children[i]);
                readB(f, n);
                if (k > n.keys[i])
                    i++;
            }
            insNonFull(n.children[i], k, data);
        }
    }

    bool srch(const string &f, long long k, string &out)
    {
        BNode n;
        readB(f, n);
        int i = 0;
        while (i < n.numKeys && k > n.keys[i])
            i++;
        if (i < n.numKeys && k == n.keys[i])
        {
            out = n.data[i];
            return true;
        }
        if (n.isLeaf)
            return false;
        return srch(n.children[i], k, out);
    }

    void inorder(const string &f, int depth)
    {
        if (f.empty())
            return;
        BNode n;
        readB(f, n);
        for (int i = 0; i < n.numKeys; i++)
        {
            if (!n.isLeaf)
                inorder(n.children[i], depth + 1);
            cout << string(depth * 2, ' ') << getCol(n.data[i], col) << "\n";
        }
        if (!n.isLeaf)
            inorder(n.children[n.numKeys], depth + 1);
    }

    void viz(const string &f, const string &pre, bool left)
    {
        if (f.empty())
            return;
        BNode n;
        readB(f, n);
        string keys_str = "[";
        for (int i = 0; i < n.numKeys; i++)
        {
            keys_str += getCol(n.data[i], col);
            if (i < n.numKeys - 1)
                keys_str += "|";
        }
        keys_str += "]";
        cout << pre << (left ? "+-- " : "\\-- ") << keys_str << "\n";
        if (!n.isLeaf)
        {
            string np = pre + (left ? "|   " : "    ");
            for (int i = 0; i <= n.numKeys; i++)
                viz(n.children[i], np, i < n.numKeys);
        }
    }

    string merkle(const string &f)
    {
        if (f.empty())
            return string(16, '0');
        BNode n;
        readB(f, n);
        string combined;
        for (int i = 0; i < n.numKeys; i++)
        {
            if (!n.isLeaf)
                combined += merkle(n.children[i]);
            combined += n.data[i];
        }
        if (!n.isLeaf)
            combined += merkle(n.children[n.numKeys]);
        return hashData(combined);
    }

public:
    void init(const string &dir, int c, int order)
    {
        folder = dir;
        root = "";
        col = c;
        t = order;
        createDir(folder);
    }
    void setFolder(const string &d) { folder = d; }
    void setRoot(const string &r) { root = r; }
    void setCnt(int c) { cnt = c; }
    string getRoot() { return root; }
    int getCnt() { return cnt; }

    void insert(const string &data, int /*row*/)
    {
        long long k = instructorHash(getCol(data, col));
        if (root.empty())
        {
            root = newFile();
            BNode n;
            n.isLeaf = true;
            n.numKeys = 1;
            n.keys.push_back(k);
            n.data.push_back(data);
            writeB(root, n);
        }
        else
        {
            BNode r;
            readB(root, r);
            if (r.numKeys == 2 * t - 1)
            {
                string nr = newFile();
                BNode nrn;
                nrn.isLeaf = false;
                nrn.numKeys = 0;
                nrn.children.push_back(root);
                writeB(nr, nrn);
                splitChild(nr, 0, root);
                root = nr;
            }
            insNonFull(root, k, data);
        }
    }

    bool search(const string &keyVal, string &out)
    {
        if (root.empty())
            return false;
        return srch(root, instructorHash(keyVal), out);
    }

    void printInorder()
    {
        if (root.empty())
        {
            cout << "  (empty)\n";
            return;
        }
        inorder(root, 0);
    }
    void visualize()
    {
        if (root.empty())
        {
            cout << "  (empty)\n";
            return;
        }
        BNode n;
        readB(root, n);
        string ks = "[";
        for (int i = 0; i < n.numKeys; i++)
        {
            ks += getCol(n.data[i], col);
            if (i < n.numKeys - 1)
                ks += "|";
        }
        ks += "]";
        cout << ks << "\n";
        if (!n.isLeaf)
            for (int i = 0; i <= n.numKeys; i++)
                viz(n.children[i], "", i < n.numKeys);
    }
    string merkleRoot()
    {
        if (root.empty())
            return string(16, '0');
        return merkle(root);
    }
};

// commit structure and repo metadata

struct Commit
{
    int id;
    string message;
    string timestamp;
    string merkle;
    string treeType;
};

struct RepoMeta
{
    string treeType; // "AVL", "RB", "B"
    int colIndex = 1;
    int bOrder = 2;
    string csvFile;
    string currentBranch;
    vector<string> branches;
    string rootFile = "NULL";
    int rowCount = 0;
    int nextRow = 0;
    int bCnt = 0;
    vector<string> colNames;
};

enum TreeMode
{
    NONE,
    AVL_TREE,
    RB_TREE,
    B_TREE
};

RepoMeta gMeta;
AVLTree gAVL;
RBTree gRB;
BTree gBT;
TreeMode gMode = NONE;

// branch folder name
string bfolder(const string &name) { return "gitlite_" + name; }

// save/load metadata and commits
void saveMeta(const string &dir)
{
    ofstream f(dir + "/META.gitlite");
    f << "TREE:" << gMeta.treeType << "\n"
      << "COL:" << gMeta.colIndex << "\n"
      << "ORDER:" << gMeta.bOrder << "\n"
      << "CSV:" << gMeta.csvFile << "\n"
      << "BRANCH:" << gMeta.currentBranch << "\n"
      << "ROOT:" << gMeta.rootFile << "\n"
      << "ROWS:" << gMeta.rowCount << "\n"
      << "NEXTROW:" << gMeta.nextRow << "\n"
      << "BCNT:" << gMeta.bCnt << "\n"
      << "BRANCHES:" << gMeta.branches.size() << "\n";
    for (auto &b : gMeta.branches)
        f << b << "\n";
    f << "COLS:" << gMeta.colNames.size() << "\n";
    for (auto &c : gMeta.colNames)
        f << c << "\n";
}

bool loadMeta(const string &dir)
{
    ifstream f(dir + "/META.gitlite");
    if (!f.is_open())
        return false;
    string line;
    auto val = [&](string &out)
    {
        getline(f, line);
        size_t p = line.find(':');
        out = (p != string::npos) ? line.substr(p + 1) : "";
    };
    val(gMeta.treeType);
    {
        string v;
        val(v);
        gMeta.colIndex = stoi(v);
    }
    {
        string v;
        val(v);
        gMeta.bOrder = stoi(v);
    }
    val(gMeta.csvFile);
    val(gMeta.currentBranch);
    val(gMeta.rootFile);
    {
        string v;
        val(v);
        gMeta.rowCount = stoi(v);
    }
    {
        string v;
        val(v);
        gMeta.nextRow = stoi(v);
    }
    {
        string v;
        val(v);
        gMeta.bCnt = stoi(v);
    }
    {
        string v;
        val(v);
        int n = stoi(v);
        gMeta.branches.clear();
        for (int i = 0; i < n; i++)
        {
            getline(f, line);
            gMeta.branches.push_back(line);
        }
    }
    {
        string v;
        val(v);
        int n = stoi(v);
        gMeta.colNames.clear();
        for (int i = 0; i < n; i++)
        {
            getline(f, line);
            gMeta.colNames.push_back(line);
        }
    }
    return true;
}

void saveCommit(const string &dir, const Commit &c)
{
    ofstream f(makeCommitLogPath(dir), ios::app);
    f << "COMMIT#" << c.id << "\n"
      << "MSG:" << c.message << "\n"
      << "TIME:" << c.timestamp << "\n"
      << "MERKLE:" << c.merkle << "\n"
      << "TREE:" << c.treeType << "\n"
      << "---\n";
}

vector<Commit> loadCommits(const string &dir)
{
    vector<Commit> v;
    ifstream f(makeCommitLogPath(dir));
    if (!f.is_open())
        return v;
    string line;
    while (getline(f, line))
    {
        if (line.find("COMMIT#") == 0)
        {
            Commit c;
            c.id = stoi(line.substr(7));
            getline(f, line);
            c.message = line.substr(4);
            getline(f, line);
            c.timestamp = line.substr(5);
            getline(f, line);
            c.merkle = line.substr(7);
            getline(f, line);
            c.treeType = line.substr(5);
            getline(f, line); // "---"
            v.push_back(c);
        }
    }
    return v;
}

// tree conversion and Merkle root
void treeToMeta()
{
    if (gMode == AVL_TREE)
        gMeta.rootFile = gAVL.getRoot();
    else if (gMode == RB_TREE)
        gMeta.rootFile = gRB.getRoot();
    else if (gMode == B_TREE)
    {
        gMeta.rootFile = gBT.getRoot();
        gMeta.bCnt = gBT.getCnt();
    }
}

void metaToTree(const string &dir)
{
    if (gMode == AVL_TREE)
    {
        gAVL.setFolder(dir);
        gAVL.setCol(gMeta.colIndex);
        gAVL.setRoot(gMeta.rootFile);
    }
    else if (gMode == RB_TREE)
    {
        gRB.setFolder(dir);
        gRB.setCol(gMeta.colIndex);
        gRB.setRoot(gMeta.rootFile);
    }
    else if (gMode == B_TREE)
    {
        gBT.setFolder(dir);
        gBT.setCnt(gMeta.bCnt);
        gBT.setRoot(gMeta.rootFile);
    }
}

string currentMerkle()
{
    if (gMode == AVL_TREE)
        return gAVL.merkleRoot();
    if (gMode == RB_TREE)
        return gRB.merkleRoot();
    return gBT.merkleRoot();
}

// command implementations

void cmdInit(const string &csvFile)
{
    if (!fileExists(csvFile))
    {
        cout << "Error: file not found: " << csvFile << "\n";
        return;
    }

    ifstream csv(csvFile);
    string header;
    getline(csv, header);
    gMeta.colNames = splitCSV(header);
    if (gMeta.colNames.empty())
    {
        cout << "Error: empty CSV.\n";
        return;
    }

    cout << "\nColumns:\n";
    for (int i = 0; i < (int)gMeta.colNames.size(); i++)
        cout << "  [" << (i + 1) << "] " << gMeta.colNames[i] << "\n";

    int col = 0;
    while (col < 1 || col > (int)gMeta.colNames.size())
    {
        cout << "Select key column (1-" << gMeta.colNames.size() << "): ";
        string s;
        getline(cin, s);
        try
        {
            col = stoi(trim(s));
        }
        catch (...)
        {
        }
    }

    cout << "\nTree type:\n  [1] AVL\n  [2] Red-Black\n  [3] B-Tree\n";
    int tc = 0;
    while (tc < 1 || tc > 3)
    {
        cout << "Choice: ";
        string s;
        getline(cin, s);
        try
        {
            tc = stoi(trim(s));
        }
        catch (...)
        {
        }
    }

    int bOrder = 2;
    if (tc == 3)
    {
        bOrder = 0;
        while (bOrder < 2)
        {
            cout << "B-Tree minimum degree t (>=2): ";
            string s;
            getline(cin, s);
            try
            {
                bOrder = stoi(trim(s));
            }
            catch (...)
            {
            }
        }
    }

    int maxRows = -1;
    cout << "Max rows to load (-1 = all): ";
    {
        string s;
        getline(cin, s);
        try
        {
            maxRows = stoi(trim(s));
        }
        catch (...)
        {
        }
    }

    // setup
    vector<string> savedColNames = gMeta.colNames;
    gMeta = RepoMeta();
    gMeta.colNames = savedColNames;
    gMeta.csvFile = csvFile;
    gMeta.colIndex = col;
    gMeta.bOrder = bOrder;
    gMeta.currentBranch = "main";
    gMeta.branches = {"main"};

    if (tc == 1)
    {
        gMeta.treeType = "AVL";
        gMode = AVL_TREE;
    }
    else if (tc == 2)
    {
        gMeta.treeType = "RB";
        gMode = RB_TREE;
    }
    else
    {
        gMeta.treeType = "B";
        gMode = B_TREE;
    }

    string dir = bfolder("main");
    createDir(dir);
    if (gMode == AVL_TREE)
        gAVL.init(dir, col);
    else if (gMode == RB_TREE)
        gRB.init(dir, col);
    else
        gBT.init(dir, col, bOrder);

    cout << "\nLoading...\n";
    string row;
    int rowNum = 0, loaded = 0;
    while (getline(csv, row))
    {
        if (row.empty())
            continue;
        if (maxRows != -1 && loaded >= maxRows)
            break;
        if (gMode == AVL_TREE)
            gAVL.insert(trim(row), rowNum);
        else if (gMode == RB_TREE)
            gRB.insert(trim(row), rowNum);
        else
            gBT.insert(trim(row), rowNum);
        rowNum++;
        loaded++;
        if (loaded % 200 == 0)
            cout << "\r  Inserted " << loaded << " rows..." << flush;
    }
    cout << "\r  Inserted " << loaded << " rows.      \n";

    gMeta.rowCount = loaded;
    gMeta.nextRow = rowNum;
    treeToMeta();

    Commit c;
    c.id = 1;
    c.message = "Initial commit";
    c.timestamp = getCurrentTimestamp();
    c.treeType = gMeta.treeType;
    c.merkle = currentMerkle();
    saveCommit(dir, c);
    saveMeta(dir);

    cout << "Done. Branch 'main' | Tree: " << gMeta.treeType
         << " | Key: " << gMeta.colNames[col - 1] << " | Rows: " << loaded << "\n";
}

void cmdCommit(const string &msg)
{
    if (gMode == NONE)
    {
        cout << "Error: no repo. Use 'init' first.\n";
        return;
    }
    if (msg.empty())
    {
        cout << "Error: message required.\n";
        return;
    }
    string dir = bfolder(gMeta.currentBranch);
    treeToMeta();
    saveMeta(dir);
    auto commits = loadCommits(dir);
    Commit c;
    c.id = (int)commits.size() + 1;
    c.message = msg;
    c.timestamp = getCurrentTimestamp();
    c.treeType = gMeta.treeType;
    c.merkle = currentMerkle();
    saveCommit(dir, c);
    cout << "Committed #" << c.id << ": \"" << msg << "\"\n";
}

void cmdBranch(const string &name)
{
    if (gMode == NONE)
    {
        cout << "Error: no repo.\n";
        return;
    }
    for (auto &b : gMeta.branches)
        if (b == name)
        {
            cout << "Branch already exists.\n";
            return;
        }
    string src = bfolder(gMeta.currentBranch), dst = bfolder(name);
    treeToMeta();
    saveMeta(src);
    copyDirRecursive(src, dst);
    string old = gMeta.currentBranch;
    gMeta.currentBranch = name;
    gMeta.branches.push_back(name);
    saveMeta(dst);
    gMeta.currentBranch = old;
    gMeta.branches.push_back(name);
    saveMeta(src);
    // remove duplicate
    sort(gMeta.branches.begin(), gMeta.branches.end());
    gMeta.branches.erase(unique(gMeta.branches.begin(), gMeta.branches.end()), gMeta.branches.end());
    cout << "Branch '" << name << "' created from '" << old << "'.\n";
}

void cmdCheckout(const string &name)
{
    if (gMode == NONE)
    {
        cout << "Error: no repo.\n";
        return;
    }
    bool found = false;
    for (auto &b : gMeta.branches)
        if (b == name)
        {
            found = true;
            break;
        }
    if (!found)
    {
        cout << "Branch '" << name << "' not found.\n";
        return;
    }
    treeToMeta();
    saveMeta(bfolder(gMeta.currentBranch));
    gMeta.currentBranch = name;
    if (!loadMeta(bfolder(name)))
    {
        cout << "Error loading branch.\n";
        return;
    }
    if (gMeta.treeType == "AVL")
        gMode = AVL_TREE;
    else if (gMeta.treeType == "RB")
        gMode = RB_TREE;
    else
        gMode = B_TREE;
    if (gMode == B_TREE)
        gBT.init(bfolder(name), gMeta.colIndex, gMeta.bOrder);
    metaToTree(bfolder(name));
    cout << "Switched to branch '" << name << "'.\n";
}

void cmdBranches()
{
    if (gMeta.branches.empty())
    {
        cout << "No branches.\n";
        return;
    }
    cout << "\nBranches:\n";
    for (auto &b : gMeta.branches)
        cout << (b == gMeta.currentBranch ? "  * " : "    ") << b << "\n";
    cout << "\n";
}

void cmdDeleteBranch(const string &name)
{
    if (name == "main")
    {
        cout << "Cannot delete 'main'.\n";
        return;
    }
    if (name == gMeta.currentBranch)
    {
        cout << "Cannot delete active branch.\n";
        return;
    }
    bool found = false;
    for (auto it = gMeta.branches.begin(); it != gMeta.branches.end(); ++it)
        if (*it == name)
        {
            gMeta.branches.erase(it);
            found = true;
            break;
        }
    if (!found)
    {
        cout << "Branch '" << name << "' not found.\n";
        return;
    }
    try
    {
        fs::remove_all(bfolder(name));
    }
    catch (...)
    {
    }
    saveMeta(bfolder(gMeta.currentBranch));
    cout << "Branch '" << name << "' deleted.\n";
}

void cmdMerge(const string &src, const string &dst)
{
    bool sf = false, df = false;
    for (auto &b : gMeta.branches)
    {
        if (b == src)
            sf = true;
        if (b == dst)
            df = true;
    }
    if (!sf)
    {
        cout << "Source branch not found.\n";
        return;
    }
    if (!df)
    {
        cout << "Dest branch not found.\n";
        return;
    }
    try
    {
        for (auto &e : fs::directory_iterator(bfolder(src)))
        {
            string fn = e.path().filename().string();
            if (fn.find(".node") != string::npos)
                fs::copy_file(e.path(), bfolder(dst) + "/" + fn, fs::copy_options::overwrite_existing);
        }
    }
    catch (exception &e)
    {
        cout << "Merge error: " << e.what() << "\n";
        return;
    }
    if (dst == gMeta.currentBranch)
    {
        loadMeta(bfolder(dst));
        metaToTree(bfolder(dst));
    }
    Commit c;
    auto commits = loadCommits(bfolder(dst));
    c.id = (int)commits.size() + 1;
    c.message = "Merged '" + src + "' into '" + dst + "'";
    c.timestamp = getCurrentTimestamp();
    c.treeType = gMeta.treeType;
    c.merkle = "MERGED";
    saveCommit(bfolder(dst), c);
    cout << "Merged '" << src << "' into '" << dst << "'.\n";
}

void cmdLog()
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    auto commits = loadCommits(bfolder(gMeta.currentBranch));
    if (commits.empty())
    {
        cout << "No commits yet.\n";
        return;
    }
    cout << "\nCommit History [" << gMeta.currentBranch << "]:\n";
    cout << string(50, '-') << "\n";
    for (auto it = commits.rbegin(); it != commits.rend(); ++it)
    {
        cout << "Commit #" << it->id << "  " << it->timestamp << "\n";
        cout << "  " << it->message << "\n";
        cout << "  Merkle: " << it->merkle.substr(0, 32) << "...\n\n";
    }
    cout << string(50, '-') << "\n";
}

void cmdAdd(const string &row)
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    if (row.empty())
    {
        cout << "Data required.\n";
        return;
    }
    int rn = gMeta.nextRow++;
    if (gMode == AVL_TREE)
        gAVL.insert(row, rn);
    else if (gMode == RB_TREE)
        gRB.insert(row, rn);
    else
        gBT.insert(row, rn);
    gMeta.rowCount++;
    treeToMeta();
    saveMeta(bfolder(gMeta.currentBranch));
    cout << "Record inserted.\n";
}

void cmdDelete(const string &keyVal)
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    if (gMode == AVL_TREE)
    {
        gAVL.remove(keyVal);
        treeToMeta();
        saveMeta(bfolder(gMeta.currentBranch));
        cout << "Node deleted.\n";
    }
    else
    {
        cout << "Delete only implemented for AVL in this version.\n";
    }
}

void cmdSearch(const string &keyVal)
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    string out;
    bool found = false;
    if (gMode == AVL_TREE)
        found = gAVL.search(keyVal, out);
    else if (gMode == RB_TREE)
        found = gRB.search(keyVal, out);
    else
        found = gBT.search(keyVal, out);
    if (!found)
    {
        cout << "Not found: " << keyVal << "\n";
        return;
    }
    cout << "\nRecord found:\n";
    auto cols = splitCSV(out);
    for (int i = 0; i < (int)cols.size(); i++)
    {
        string name = (i < (int)gMeta.colNames.size()) ? gMeta.colNames[i] : ("Col" + to_string(i + 1));
        cout << "  " << left << setw(20) << name << ": " << cols[i] << "\n";
    }
    cout << "\n";
}

void cmdUpdate(const string &keyVal, int colNum, const string &newVal)
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    string out;
    bool found = false;
    if (gMode == AVL_TREE)
        found = gAVL.search(keyVal, out);
    else if (gMode == RB_TREE)
        found = gRB.search(keyVal, out);
    else
        found = gBT.search(keyVal, out);
    if (!found)
    {
        cout << "Not found.\n";
        return;
    }
    auto cols = splitCSV(out);
    if (colNum < 1 || colNum > (int)cols.size())
    {
        cout << "Column out of range.\n";
        return;
    }
    cols[colNum - 1] = newVal;
    string newRow;
    for (int i = 0; i < (int)cols.size(); i++)
    {
        if (i)
            newRow += ",";
        newRow += cols[i];
    }
    if (gMode == AVL_TREE)
    {
        gAVL.remove(keyVal);
        gAVL.insert(newRow, gMeta.nextRow++);
    }
    else if (gMode == RB_TREE)
    {
        gRB.insert(newRow, gMeta.nextRow++);
        cout << "(Note: old record may persist in RB tree)\n";
    }
    else
    {
        gBT.insert(newRow, gMeta.nextRow++);
    }
    gMeta.rowCount++;
    treeToMeta();
    saveMeta(bfolder(gMeta.currentBranch));
    cout << "Updated.\n";
}

void cmdVisualize()
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    cout << "\nTree [" << gMeta.treeType << "] Branch: " << gMeta.currentBranch
         << " | Key: ";
    if (!gMeta.colNames.empty())
        cout << gMeta.colNames[gMeta.colIndex - 1];
    cout << "\n"
         << string(40, '-') << "\n";
    if (gMode == AVL_TREE)
        gAVL.visualize();
    else if (gMode == RB_TREE)
        gRB.visualize();
    else
        gBT.visualize();
    cout << "\n";
}

void cmdPrint()
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    cout << "\nRecords (sorted):\n"
         << string(40, '-') << "\n";
    if (gMode == AVL_TREE)
        gAVL.printInorder();
    else if (gMode == RB_TREE)
        gRB.printInorder();
    else
        gBT.printInorder();
    cout << "\n";
}

void cmdVerify()
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    cout << "Computing Merkle root...\n";
    string cur = currentMerkle();
    auto commits = loadCommits(bfolder(gMeta.currentBranch));
    if (commits.empty())
    {
        cout << "Current Merkle: " << cur << "\nNo commits to compare.\n";
        return;
    }
    const auto &last = commits.back();
    cout << "Last commit: " << last.merkle << "\n";
    cout << "Current:     " << cur << "\n";
    if (cur == last.merkle || last.merkle == "MERGED")
        cout << "OK - data integrity verified.\n";
    else
        cout << "WARNING - Merkle mismatch! Data may have changed since last commit.\n";
}

void cmdSave()
{
    if (gMode == NONE)
    {
        cout << "No repo.\n";
        return;
    }
    treeToMeta();
    saveMeta(bfolder(gMeta.currentBranch));
    cout << "Saved to: " << bfolder(gMeta.currentBranch) << "\n";
}

void cmdLoad(const string &name)
{
    string dir = bfolder(name);
    if (!fileExists(dir + "/META.gitlite"))
    {
        cout << "Branch not found: " << name << "\n";
        return;
    }
    cmdCheckout(name);
    cout << "Loaded branch '" << name << "'.\n";
}

// command parser

void dispatch(const string &input)
{
    if (input.empty())
        return;

    // Tokenize, respecting quoted strings
    vector<string> tok;
    string cur;
    bool inQ = false;
    for (char c : input)
    {
        if (c == '"')
            inQ = !inQ;
        else if (c == ' ' && !inQ)
        {
            if (!cur.empty())
            {
                tok.push_back(cur);
                cur = "";
            }
        }
        else
            cur += c;
    }
    if (!cur.empty())
        tok.push_back(cur);
    if (tok.empty())
        return;

    string cmd = tok[0];
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    auto ask = [](const string &prompt)
    {
        cout << prompt;
        string s;
        getline(cin, s);
        return trim(s);
    };

    if (cmd == "init")
    {
        string f = (tok.size() > 1) ? tok[1] : ask("CSV file: ");
        cmdInit(f);
    }
    else if (cmd == "commit")
    {
        string msg = "";
        for (size_t i = 1; i < tok.size(); i++)
        {
            if (i > 1)
                msg += " ";
            msg += tok[i];
        }
        if (msg.empty())
            msg = ask("Commit message: ");
        cmdCommit(msg);
    }
    else if (cmd == "branch")
    {
        string n = (tok.size() > 1) ? tok[1] : ask("Branch name: ");
        cmdBranch(n);
    }
    else if (cmd == "checkout")
    {
        string n = (tok.size() > 1) ? tok[1] : ask("Branch name: ");
        cmdCheckout(n);
    }
    else if (cmd == "branches")
    {
        cmdBranches();
    }
    else if (cmd == "current-branch")
    {
        cout << "On branch: " << gMeta.currentBranch << "\n";
    }
    else if (cmd == "delete-branch")
    {
        string n = (tok.size() > 1) ? tok[1] : ask("Branch to delete: ");
        cmdDeleteBranch(n);
    }
    else if (cmd == "merge")
    {
        string src = (tok.size() > 1) ? tok[1] : ask("Source branch: ");
        string dst = (tok.size() > 2) ? tok[2] : ask("Target branch: ");
        cmdMerge(src, dst);
    }
    else if (cmd == "log")
    {
        cmdLog();
    }
    else if (cmd == "add")
    {
        string row = "";
        for (size_t i = 1; i < tok.size(); i++)
        {
            if (i > 1)
                row += " ";
            row += tok[i];
        }
        if (row.empty())
            row = ask("CSV row data: ");
        cmdAdd(row);
    }
    else if (cmd == "delete")
    {
        string k = (tok.size() > 1) ? tok[1] : ask("Key value: ");
        cmdDelete(k);
    }
    else if (cmd == "search")
    {
        string k = (tok.size() > 1) ? tok[1] : ask("Key value: ");
        cmdSearch(k);
    }
    else if (cmd == "update")
    {
        string k = (tok.size() > 1) ? tok[1] : ask("Key value: ");
        int col = 0;
        if (tok.size() > 2)
        {
            try
            {
                col = stoi(tok[2]);
            }
            catch (...)
            {
            }
        }
        if (col <= 0)
        {
            try
            {
                col = stoi(ask("Column number: "));
            }
            catch (...)
            {
                col = 1;
            }
        }
        string v = (tok.size() > 3) ? tok[3] : ask("New value: ");
        cmdUpdate(k, col, v);
    }
    else if (cmd == "visualize-tree" || cmd == "visualize")
    {
        cmdVisualize();
    }
    else if (cmd == "print")
    {
        cmdPrint();
    }
    else if (cmd == "verify")
    {
        cmdVerify();
    }
    else if (cmd == "save")
    {
        cmdSave();
    }
    else if (cmd == "load")
    {
        string n = (tok.size() > 1) ? tok[1] : ask("Branch name: ");
        cmdLoad(n);
    }
    else if (cmd == "help")
    {
        cout << left << "\n"
             << setw(60) << setfill('-') << "" << "\n"
             << setfill(' ') << setw(30) << "Command" << setw(30) << "Description" << "\n"
             << setw(60) << setfill('-') << "" << "\n"
             << setfill(' ') << setw(30) << "init <csv>" << setw(30) << "Initialize repo" << "\n"
             << setw(30) << "commit <message>" << setw(30) << "Save current state" << "\n"
             << setw(30) << "branch <name>" << setw(30) << "Create a branch" << "\n"
             << setw(30) << "checkout <name>" << setw(30) << "Switch branch" << "\n"
             << setw(30) << "branches" << setw(30) << "List all branches" << "\n"
             << setw(30) << "current-branch" << setw(30) << "Show active branch" << "\n"
             << setw(30) << "delete-branch <name>" << setw(30) << "Delete a branch" << "\n"
             << setw(30) << "merge <src> <dst>" << setw(30) << "Merge branches" << "\n"
             << setw(30) << "log" << setw(30) << "Commit history" << "\n"
             << setw(30) << "add <row>" << setw(30) << "Insert a record" << "\n"
             << setw(30) << "delete <key>" << setw(30) << "Delete a record (AVL only)" << "\n"
             << setw(30) << "search <key>" << setw(30) << "Search for a record" << "\n"
             << setw(30) << "update <k> <col> <v>" << setw(30) << "Update a field" << "\n"
             << setw(30) << "visualize-tree" << setw(30) << "Show tree structure" << "\n"
             << setw(30) << "print" << setw(30) << "Print all records sorted" << "\n"
             << setw(30) << "verify" << setw(30) << "Check Merkle integrity" << "\n"
             << setw(30) << "save" << setw(30) << "Save state to disk" << "\n"
             << setw(30) << "load <branch>" << setw(30) << "Load a branch" << "\n"
             << setw(30) << "exit" << setw(30) << "Quit" << "\n"
             << setw(60) << setfill('-') << "" << setfill(' ') << "\n\n";
    }
    else if (cmd == "exit" || cmd == "quit")
    {
        if (gMode != NONE)
        {
            treeToMeta();
            saveMeta(bfolder(gMeta.currentBranch));
        }
        cout << "Goodbye.\n";
        exit(0);
    }
    else
    {
        cout << "Unknown command: " << cmd << "  (type 'help')\n";
    }
}

// MAIN ftn
int main()
{
    // resume if a repo already exists
    if (fileExists("gitlite_main/META.gitlite") && loadMeta("gitlite_main"))
    {
        if (gMeta.treeType == "AVL")
            gMode = AVL_TREE;
        else if (gMeta.treeType == "RB")
            gMode = RB_TREE;
        else
            gMode = B_TREE;
        if (gMode == B_TREE)
            gBT.init(bfolder(gMeta.currentBranch), gMeta.colIndex, gMeta.bOrder);
        metaToTree(bfolder(gMeta.currentBranch));
        cout << "GitLite - Resumed branch '" << gMeta.currentBranch
             << "' | Tree: " << gMeta.treeType
             << " | Records: " << gMeta.rowCount << "\n";
    }
    else
    {
        cout << "GitLite - Version Control System\n";
        cout << "Type 'help' for commands, 'init <csv>' to start.\n";
    }

    while (true)
    {
        string branch = gMeta.currentBranch.empty() ? "" : " (" + gMeta.currentBranch + ")";
        cout << "\ngitlite" << branch << " > ";
        string line;
        getline(cin, line);
        dispatch(trim(line));
    }
}