/**
 * @file k_way_merge_sorter.hpp
 * @brief K-путевая внешняя сортировка слиянием
 * @author External Sort Library
 * @version 1.0
 */

#pragma once

#include "debug_logger.hpp"
#include "interfaces.hpp"

#include <functional>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

namespace external_sort {

/**
 * @brief Структура, представляющая источник данных для слияния
 *
 * Используется в приоритетной очереди для k-путевого слияния.
 *
 * @tparam T Тип элементов данных
 */
template <typename T>
struct MergeSource {
    T value;                  ///< Значение из источника
    IInputStream<T>* stream;  ///< Указатель на поток-источник

    /**
     * @brief Оператор сравнения для приоритетной очереди
     * @param other Другой источник для сравнения
     * @return true, если этот источник больше другого
     */
    bool operator>(const MergeSource<T>& other) const {
        return value > other.value;
    }
};

/**
 * @brief Шаблонный класс для выполнения k-путевой внешней сортировки слиянием
 *
 * Класс абстрагирован от конкретной реализации хранилища данных и работает
 * через интерфейсы IStreamFactory, IInputStream и IOutputStream.
 *
 * @tparam T Тип элементов для сортировки
 */
template <typename T>
class KWayMergeSorter {
   private:
    IStreamFactory<T>& stream_factory_;  ///< Ссылка на фабрику потоков
    StorageId input_id_;                 ///< Идентификатор входного хранилища
    StorageId output_id_;                ///< Идентификатор выходного хранилища
    uint64_t memory_for_runs_bytes_;     ///< Память для создания начальных прогонов (байты)
    uint64_t k_way_degree_;              ///< Степень k-путевого слияния
    uint64_t file_io_buffer_elements_;   ///< Размер буфера ввода-вывода в элементах
    bool ascending_;                     ///< Флаг сортировки по возрастанию

    /**
     * @brief Компаратор для источников слияния
     */
    struct MergeSourceComparator {
        bool ascending;  ///< Флаг направления сортировки

        /**
         * @brief Конструктор компаратора
         * @param asc Направление сортировки (true - по возрастанию)
         */
        explicit MergeSourceComparator(bool asc) : ascending(asc) {
        }

        /**
         * @brief Оператор сравнения
         * @param a Первый источник
         * @param b Второй источник
         * @return Результат сравнения для приоритетной очереди
         */
        bool operator()(const MergeSource<T>& a, const MergeSource<T>& b) const {
            return ascending ? (a.value > b.value) : (a.value < b.value);
        }
    };

    /**
     * @brief Создает начальные отсортированные прогоны из входного потока
     * @return Вектор идентификаторов созданных прогонов
     */
    std::vector<StorageId> CreateInitialRuns();

    /**
     * @brief Объединяет группу прогонов в один
     * @param group_run_ids Идентификаторы прогонов для объединения
     * @param output_run_id Идентификатор выходного прогона
     */
    void MergeGroupOfRuns(
        const std::vector<StorageId>& group_run_ids, const StorageId& output_run_id);

   public:
    /**
     * @brief Конструктор k-путевого сортировщика слиянием
     * @param factory Ссылка на фабрику потоков
     * @param input_id Идентификатор входного хранилища
     * @param output_id Идентификатор выходного хранилища
     * @param mem_bytes Память для создания прогонов в байтах
     * @param k_degree Степень k-путевого слияния (минимум 2)
     * @param io_buf_elems Размер буфера ввода-вывода в элементах
     * @param sort_ascending Направление сортировки (по умолчанию по возрастанию)
     * @throws std::invalid_argument если k_degree < 2
     * @throws std::runtime_error если output_id находится в контексте временных файлов
     */
    KWayMergeSorter(
        IStreamFactory<T>& factory, StorageId input_id, StorageId output_id, uint64_t mem_bytes,
        uint64_t k_degree, uint64_t io_buf_elems, bool sort_ascending = true);

    /**
     * @brief Выполняет сортировку
     *
     * Основной метод, который выполняет полную k-путевую внешнюю сортировку:
     * 1. Создает начальные отсортированные прогоны
     * 2. Многократно объединяет прогоны до получения одного
     * 3. Делает финальный прогон постоянным с именем output_id
     */
    void Sort();
};

#include "impl/k_way_merge_sorter.tpp"

}  // namespace external_sort
