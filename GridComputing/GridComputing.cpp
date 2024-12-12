#include <iostream>
#include <cmath>
#include <vector>
#include <mpi.h>
#include <windows.h>
#include <winsock.h>

using namespace std;

// Hàm kiểm tra tính nguyên tố của một số
bool is_prime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i <= std::sqrt(n); ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

// Hàm tính số nguyên tố Mersenne
double mersenne_prime(int p) {
    double mp = pow(2, p) - 1; // Tính M_p = 2^p - 1
    if (is_prime(mp)) {
        return mp; // Nếu là số nguyên tố, trả về giá trị M_p
    }
    return -1; // Trả về -1 nếu không phải số nguyên tố
}

int main(int argc, char** argv) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    char hostname[256];
    WSACleanup();
    MPI_Init(&argc, &argv); // Khởi tạo MPI

    int rank, size, rank2;
    //MPI_COMM_WORLD là một communicator mặc định trong MPI, chứa tất cả các tiến trình được khởi tạo trong chương trình
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Lấy rank của tiến trình hiện tại
    MPI_Comm_size(MPI_COMM_WORLD, &size); // Lấy tổng số tiến trình
    MPI_Comm_dup(MPI_COMM_WORLD, &rank2);
    // Định nghĩa khoảng số p để kiểm tra, trên máy hiện tại được phân từ 1-10
    int start_p = 1;
    int end_p = 10;

    // Tính toán phân chia công việc giữa các tiến trình, để phòng trường hợp chạy nhiều tiến trình, còn không có thể xóa
    int step = (end_p - start_p + 1) / size;
    int local_start = start_p + rank * step;
    int local_end = (rank == size - 1) ? end_p + 1 : local_start + step;

    // Mảng lưu kết quả của mỗi tiến trình
    vector < double > local_results;

    // Đo thời gian bắt đầu
    double start_time = MPI_Wtime();

    // Mỗi tiến trình tính toán số Mersenne trong khoảng của nó
    for (int p = local_start; p < local_end; ++p) {
        double mp = mersenne_prime(p);
        if (mp != -1) {
            local_results.push_back(mp);
        }
    }

    // Đo thời gian kết thúc
    double end_time = MPI_Wtime();
    double elapsed_time = end_time - start_time;

    // Gửi kích thước dữ liệu về rank 0
    int local_size = local_results.size();
    vector < int > recvcounts(size);
    MPI_Gather(&local_size, 1, MPI_INT, recvcounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Tính toán vị trí nhận dữ liệu tại rank 0
    vector < int > displs(size, 0);
    if (rank == 0) {
        for (int i = 1; i < size; ++i) {
            displs[i] = displs[i - 1] + recvcounts[i - 1];
        }
    }

    // Tổng kích thước dữ liệu cần nhận tại rank 0
    int total_results = displs[size - 1] + recvcounts[size - 1];
    vector < int > all_results(total_results);

    // Thu thập kết quả từ các tiến trình về rank 0
    MPI_Gatherv(local_results.data(), local_size, MPI_INT,
        all_results.data(), recvcounts.data(), displs.data(), MPI_INT,
        0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    // Tiến trình chính (rank 0) in kết quả
    if (rank == 0) {
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            cout << "May " << hostname << " tim duoc cac so Mersenne voi p trong khoang tu " << local_start << " den " << local_end << " la: ";
            for (int mp : local_results) {
                cout << mp << " ";
            }
            cout << endl;
            // In thời gian thực hiện của từng tiến trìnha
            cout << "Tong thoi gian hoan thanh tinh toan la: " << fixed << "\t" << elapsed_time << " giay" << endl;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD); // Đồng bộ hóa giữa các tiến trình
    MPI_Finalize(); // Kết thúc MPI
    return 0;
}