#include <queue>

template <typename T> class IrqFifo {
    public: 
        IrqFifo() : m_fifo(), empty(true) {}

        void push(T msg) {
            //TODO add mutex
            m_fifo.push(msg);
            this->empty = false;
        }

        bool hasMessages() const {
            return !empty;
        }
        
        T pop() {
            //TODO add mutex
            T msg = m_fifo.front();
            m_fifo.pop();
            this->empty = m_fifo.empty();
            return msg;
        }

    private:
        std::queue<T> m_fifo;
        volatile bool empty;
};
