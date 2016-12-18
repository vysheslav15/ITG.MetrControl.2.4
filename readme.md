[![Build status](https://ci.appveyor.com/api/projects/status/26eki19xnd70arbg?svg=true)](https://ci.appveyor.com/project/sergey-s-betke/itg-metrcontrol-2-4)

Метроконтроль 2.4 - .msi пакет
==============================

Данный проект - .msi пакет для развёртывания в рамках домена продукта АИС Метроконтроль версии 2.4. 

Инструменты для сборки .msi пакета
----------------------------------

Для внесения изменений в пакет и повторной сборки пакета потребуются следующие продукты:

- [Microsoft Visual Studio 2015 Community Edition][VS]
- [Windows Installer XML Toolset - WIX][WIX]

Открываем файл решения `Metrocontrol\Metrocontrol.sln` и собираем решение.

Варианты пакетов
----------------

### Административная точка установки

В папке `Metrocontrol\bin\Admin image\x86\ru-RU` собран проект, подготовленный к роли административной точки установки.
В нём отсутствует интерфейс пользователя.

### .msi пакет в виде единого файла

В папке `Metrocontrol\bin\Single .msi file\x86\ru-RU` собран .msi пакет в виде единого файла.
В отличии от предыдущего варианта, в данной редакции присутствует интерфейс пользователя,
позволяющий изменить и состав продукта, и папку его установки.

Подготовка административной точки установки
-------------------------------------------

При подготовке административной точки установки доступны к изменении нижеописанные свойства.

### ALLUSERS

По умолчанию - `"1"`. При установке `"0"` регистрация будет осуществлена в реестре пользователя, а не в реестре компьютера.

### DISABLESHORTCUTS

По умолчанию - `"No"`. При установке значения `"Yes"` при установке не будут опубликованы ярлыки.
Данным свойством следует воспользоваться, если Вы планируете запускать приложения АИС Метроконтроль через ярлыки на
`.csmdb23` файлы (о данных дополнительных возможностях пакета читайте далее).

### APPLICATIONFOLDER

Путь к папке, в которую будет установлен программный продукт. По умолчанию - `%ProgramFiles(x86)%\РЦН\Метроконтроль\2.3%`.
Устанавливать в качестве значения данного свойства требуется только путь по отношению к `%ProgramFiles%`.

### INSTALLLEVEL

По умолчанию - `"100"`. По умолчнию будут установлены только приложения "АИС Метроконтроль" и "Учёт клейм".
При установке `INSTALLLEVEL` больше 200 будет установлено и приложение "Метроконтроль - администратор".

### Управление установкой приложений

Для управления установкой приложений при подготовке административной точки установки следует воспользоваться
свойством `ADDDEFAULT`, перечислив приложения через запятую. Идентификаторы приложений:

- `csmmain` - собственно "АИС Метроконтроль";
- `markinv` - приложение "Учёт клейм";
- `csmadmin` - приложение "Метроконтроль - администратор".

Например, если при подготовке административной точки установки указано `ADDDEFAULT=csmadmin`,
то подготовленная точка установки без дополнительных трансформаций подзволит установить только
приложение "Метроконтроль - администратор".

### Примеры

Несколько примеров подготовки административной точки установки:

	msiexec -a Metrcontrol.msi DISABLESHORTCUTS=Yes ALLUSERS=0 ADDDEFAULT=csmmain

Данная командная строка готовит точку установки с "отключенными" ярлыками, с установкой приложения для пользователя
(а не для компьютера). Из приложений при этом будет установлена только АИС Метроконтроль.

	msiexec -a Metrcontrol.msi DISABLESHORTCUTS=Yes ADDDEFAULT=csmmain,makrinv,csmadmin

Данная командная строка готовит точку установки с "отключенными" ярлыками. Приложения будут установлены все.

Использование нескольких БД
---------------------------

В некоторых случаях необходима возможность подключаться к нескольким базам данных. При этом явно не стоит заставлять каждый раз
вводить параметры подключения к БД. Для решения данной задачи в данном пакете регистрируется новый тип файла `.csmdb23` и
ProgId `RCN.Bootstrapper.2.3`.

### Структура файла .csmdb23

Файлы с расширением `.csmdb23` далее будем называть описателем базы данных АИС Метроконтроль. Файл, по сути, представляет собой
ini файл. За основу взят формат ini с одной только целью: предоставить возможность внесения изменений в данный файл посредством
GPO+GPP.

Пример файла:

	[MetrControl]
	Version=2.4

	[MetrControlDB]
	Server=<ip-адрес или FQDN SQL сервера>
	Database=<имя базы данных>
	Description=<описание базы данных>
	NTLM=yes/no; использовать учётную запись пользователя для подключения к SQL серверу, или явно указанные учётные данные
	Login=<login>
	PasswordHash=<password hash>

Создать файл-описатель Вашей БД можно через контекстное меню проводника "Создать"-"Описатель БД АИС Метроконтроль". Для изменения
созданного / существующего файла удерживайте Shift при щелчке правой кнопкой мыши на файле / ярлыке на `.csmdb23` файл, после чего
воспользуйтесь глаголом "Изменить" (он доступен только при нажатой клавише Shift).

Вместо пароля следует сохранять хеш пароля, который следует получить из файла `CnnSettings.xml`, создаваемого АИС Метроконтроль
при подключении к БД в профиле пользователя (`%LocalAppData%\IFirst\MetrControl\CnnSettings.xml`).

### Действия с .csmdb23

При двойном щелчке на файле `.csmdb23` или на ярлыке на файл данного типа активируется PowerShell сценарий, формирующий на основе
данных из `.csmdb23` файл `CnnSettings.xml`, после чего активирует приложение "АИС Метроконтроль" (на данном этапе поддерживаются
механизмы ms installer, и перед запуском приложения будут проверены все файлы приложения, записи в реестре и так далее, при
при необходимости - приложение будет автоматически восстановлено). Аналогичным образом можно запустить приложение и для другой
базы данных, воспользовавшись ярлыком на другой файл `.csmdb23`.

Рекомендую файлы `.csmdb23` размещать на сетевом ресурсе с включенным кешированием, а через GPO+GPP публиковать только ярлыки
на данные файлы.

В контекстном меню файла `.csmdb23` присутствуют и другие глаголы: "Учёт клейм", "Метроконтроль - администратор" (доступен только
в расширенном меню - с Shift). Данные глаголы активируют, как уже понятно, соответствующие приложения.

### Дополнительные аргументы ярлыков

При создании ярлыков на `.csmdb23` файлы следует также указать и дополнительные аргументы. В частности, если после полного пути к
файлу Вы укажите `csmadmin`, то при двойном щелчке на данном ярлыке будет активировано приложение "Метроконтроль - администратор",
если параметр `markinv` - приложение "Учёт клейм", ну а `csmmain` соответствует реакции по умолчанию - запуск приложения "АИС
Метроконтроль".

Таким образом, создав один файл - описатель базы АИС Метроконтроль, мы имеем возможность через GPO+GPP на весь домен назначить 
несколько ярлыков на данный файл, при этом мы можем указать, какое приложение будет активировать ярлык по умолчанию (при двойном
щелчке на ярлыке).

[VS]: https://www.microsoft.com/ru-ru/download/details.aspx?id=48146
[WIX]: http://wixtoolset.org/
