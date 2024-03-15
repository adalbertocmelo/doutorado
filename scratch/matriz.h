#ifndef MATRIZ_H
#define MATRIZ_H

#include <vector>
#include <stdexcept>
#include <string>

using namespace std;

template<typename T>
class Row {
public:
    Row(vector<T>* elements);
	~Row();
    T& operator[](unsigned long index);
    void setColumns(unsigned long columns);
    void setOffset(unsigned long offset);
private:
    vector<T>* elements;
    unsigned long columns;
    unsigned long offset;
};

template<typename T>
Row<T>::Row(vector<T>* elements) {
    this->elements = elements;
    columns = 0;
    offset = 0;
}

template<typename T>
Row<T>::~Row() {
	elements = nullptr;
}

template<typename T>
T& Row<T>::operator[](unsigned long index) {
    if(index < columns)
        return (*elements)[offset+index];
    else
        throw out_of_range {"column index out of range! index: " + to_string(index) + " columns: " + to_string(columns)};
}

template<typename T>
void Row<T>::setColumns(unsigned long columns){
    this->columns = columns;
}

template<typename T>
void Row<T>::setOffset(unsigned long offset) {
    this->offset = offset;
}

template<typename T>
class matriz {
public:
    matriz() = default;
    matriz(unsigned long rows, initializer_list<T> init);
    Row<T> operator[](unsigned long index);
    void setDimensions(unsigned long rows, unsigned long columns);
	unsigned long getRows();
private:
    vector<T> elements;
    Row<T> row {&elements};
    unsigned long rows, columns;
};

template<typename T>
matriz<T>::matriz(unsigned long rows, initializer_list<T> init)
	:rows{rows}
{
	unsigned long size = init.size();
	this->columns = size / rows;
    row.setColumns(columns);
	elements.reserve(size);
	uninitialized_copy(init.begin(),init.end(), elements.begin());
}

template<typename T>
Row<T> matriz<T>::operator[](unsigned long index) {
    if(index < rows) {
        row.setOffset(index*columns);
        return row;
    } else {
        throw out_of_range {"row index out of range! index: " + to_string(index) + " rows: " + to_string(rows)};
    }
}

template<typename T>
void matriz<T>::setDimensions(unsigned long rows, unsigned long columns) {
    this->rows = rows;
    this->columns = columns;
    row.setColumns(columns);
    elements.resize(rows*columns);
}

template<typename T>
unsigned long matriz<T>::getRows(){
	return this->rows;
}

#endif
