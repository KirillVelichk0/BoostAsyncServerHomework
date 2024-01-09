# Асинхронный сервер на Boost.Asio
## Кратко о проекте
- данный проект был написан на 3 курсе для одного из предметов. В рамках курса надо было: 
  - написать сервер на tcp сокетах с шифрованием
  - реализовать защищенное соединение с помощью генерации ЭЦП при установке соединения (ЭЦП на основе nonce, сам сгенерированный nonce, открытый ключ. В реальном проекте лишь один из этапов установления защищенного соединения)
  - реализация диффи-хелмана и шифрование с его помощью данных сессии с использованием генератора
  - некоторый простейший функционал самого сервера, который, в общем то, не особо и интересен 
- как оно было реализовано:
  - написан протокол mywebsocket с асинхронной записью в буфер. Вначале отправляется размер сообщения в виде 4-байтового числа, потом асинхронно до готовности считывается само сообщение (реализация буфера не оптимальная, ибо лежит обычный вектор. Удаление старых данных нужно сделать лучше)
  - над MyWebSocket есть надстройка в виде класса, позволяющая отправлять и считывать данные в формате Json
  - реализован Сервер, который может анализировать содержимое полученного json и вызывать соотвествующий обработчик
  - чтобы сервер нормально работал с корутинами, заиспользовал std::enable_shared_from_this
  ## Постмортем
  Рассматривая проведенную работу, сейчас я четко осознаю, что половину, по хорошему, надо бы переписать.  
  Что можно (и нужно) сделать лучше, и как это сделать:
  - использовать либо схему с 2 буферами (чтобы как можно реже чистить один из буферов), либо использовать вместо vector std::deque. Он умеет за O(1) удалять данные из начала и конца, за те же O(1) обращаться к произвольному элементу. Поэтому, контейнер, в принципе, для поставленных целей подходит
  - обработчики сервера сейчас регистрируются коряво, в отдельном методе. SOLID не просто страдает от подобного подхода, принцип Открытости и Закрытости мертв от подобного. Чтобы починить, можно использовать схему с макросами как для тестов (генерировать обработчики с помощью макроса, который заодно зарегистрирует их в глобальном хранилище и привяжет к серверу по имени, который мы серверу и выдали)
  - БД в проекте используется постольку поскольку, суть не в нем. Но, конечно, надо boost.mysql заиспользовать под асинхронность. На тот момент горели сроки и было лень