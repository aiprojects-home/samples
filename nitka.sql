use nitka

-- In this sample we have auto service company which can provide several types of service.
-- In DB we store list of services, list of customers and their orders.
-- Of course, it's just a brief example.

-- Create table for customers

create table Customers
(
    Id bigint not null,           -- unique id
	Name varchar(max) not null,   -- customer's name
	primary key (Id)
)

-- Create table for available services

create table Services
(
    Id bigint not null,                 -- unique id
    Description varchar(max) not null,  -- service name (e.g. painting, fixing)
    Price money not null,               -- cost per one service
    primary key (Id)   
)

-- Create table for Orders

create table Orders
(
    Id bigint not null,          -- unique id
    Date datetime not null,      -- order date
    CustomerId bigint not null,  -- link to customer
    primary key (Id),
    foreign key (CustomerId) references Customers (Id) 
)

-- Create table for OrderLines

create table OrderLines
(
    Id bigint not null,         -- unique id
    Quantity int not null,      -- service count (e.g. replacing three wheels)
    OrderId bigint not null,    -- link to order
    ServiceId bigint not null,  -- link to service
    primary key (Id),
    foreign key (OrderId) references Orders (Id),
    foreign key (ServiceId) references Services(Id)
)

-- Now push some data into DB

-- We have only one customer named John Smith:

insert into Customers (Id, Name) values (1, 'John Smith')

-- We can provide three services: oil checking, transmission repair & wheel replacement

insert into Services (Id, Description, Price) values (1, 'Transmission repair', 500)
insert into Services (Id, Description, Price) values (2, 'Check oil', 50)
insert into Services (Id, Description, Price) values (3, 'Replace wheel', 150)

-- John have two orders:

insert into Orders (Id, Date, CustomerId) values (1, GETDATE(), 1)
insert into Orders (Id, Date, CustomerId) values (2, GETDATE(), 1)

-- First order is for transmission repair:

insert into OrderLines(Id, OrderId, ServiceId, Quantity) values (1, 1, 1, 1)

-- Second is for replacement of three wheels & oil check:

insert into OrderLines(Id, OrderId, ServiceId, Quantity) values (2, 2, 3, 3)
insert into OrderLines(Id, OrderId, ServiceId, Quantity) values (3, 2, 2, 1)

-- Now using 'join' we can calculate TOTAL SUM of ALL orders made by Mr. Smith:

select sum(Services.Price * OrderLines.Quantity) as 'Total price'
from Orders join Customers on Orders.CustomerId = Customers.Id 
join OrderLines on Orders.Id = OrderLines.OrderId join Services on OrderLines.ServiceId = Services.Id
where Customers.Name = 'John Smith' 

-- And here we have 1000$. Bingo!

-- Drop all in reverse order

drop table OrderLines
drop table Orders
drop table Services
drop table Customers
