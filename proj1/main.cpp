#include "tape.hpp"

int main() {
    Tape tape("tape_data.bin",16);
    tape.generate_random_file(3);

    std::cout << "File contents:\n";
    tape.display();

    tape.open(std::ios::in | std::ios::out);
    std::vector<RecordType> buffer;
    tape.read_block(0, buffer);
    tape.close();

    std::cout << "\nFirst block read (" << buffer.size() << " records):\n";
    for (size_t i = 0; i < buffer.size(); ++i) std::cout << buffer[i] << " ";
    std::cout << "\n\nReadCount=" << tape.getReadCount()
              << " WriteCount=" << tape.getWriteCount() << std::endl;
    return 0;
}
