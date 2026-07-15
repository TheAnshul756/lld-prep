#include<iostream>
#include<memory>
#include<string>
using namespace std;


class DataSource {
public:
    virtual ~DataSource() = default;
    virtual void writeData() = 0;
    virtual void readData() = 0; 
};


class FileDataSource : public DataSource {
    string filename;
public:
    FileDataSource(string fname) {
        filename = fname;
    }
    void writeData() override {
        cout << "Writing to the file " << filename << endl;
    }
    void readData() override {
        cout << "Reading from the file " << filename << endl;
    }
};

class DataSourceDecorator : public DataSource {
    unique_ptr<DataSource> wrapee;
public:
    DataSourceDecorator(unique_ptr<DataSource> w) : wrapee(move(w)) {}
    void writeData() override {
        wrapee -> writeData();
    }
    void readData() override {
        wrapee -> readData();
    }
};

class EncryptionDecorator : public DataSourceDecorator {
public:
    EncryptionDecorator(unique_ptr<DataSource> w) : DataSourceDecorator(move(w)) {}
    void writeData() override {
        cout << "Encrypting data before Writing\n";
        DataSourceDecorator::writeData();
    }
    void readData() override {
        DataSourceDecorator::readData();
        cout << "Decrypting data after reading\n";
    }
};

class CompressionDecorator : public DataSourceDecorator {
public:
    CompressionDecorator(unique_ptr<DataSource> w) : DataSourceDecorator(move(w)) {}
    void writeData() override {
        cout << "Compressing data before Writing\n";
        DataSourceDecorator::writeData();
    }
    void readData() override {
        DataSourceDecorator::readData();
        cout << "Decompressing data after reading\n";
    }
};

int main() {
    unique_ptr<DataSource> f = make_unique<FileDataSource>("A.cpp");
    f = make_unique<CompressionDecorator>(move(f));
    f = make_unique<EncryptionDecorator>(move(f));
    f -> writeData();
    cout << "\n";
    f -> readData();
}