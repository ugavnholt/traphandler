USE [UGMon]
GO

/****** Object:  Table [dbo].[Proxies]    Script Date: 04/21/2010 17:36:31 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[Proxies](
	[ProxyHostName] [nvarchar](255) NOT NULL,
	[SNMPQueueLength] [int] NOT NULL,
	[CommandQueueLength] [int] NOT NULL,
	[HeartBeatTimeoutSecs] [int] NOT NULL,
	[MaxConcurrentCommands] [int] NOT NULL,
	[ConfigUpdateIntervalSecs] [int] NOT NULL,
	[LogMinSeverity] [nvarchar](20) NOT NULL,
	[ThreshChangeTime] [datetime] NOT NULL,
 CONSTRAINT [PK_ChangeTime] PRIMARY KEY CLUSTERED 
(
	[ProxyHostName] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_SNMPQueueLength]  DEFAULT ((1000000)) FOR [SNMPQueueLength]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_CommandQueueLength]  DEFAULT ((10000)) FOR [CommandQueueLength]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_HeartBeatTimeoutSecs]  DEFAULT ((10)) FOR [HeartBeatTimeoutSecs]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_MaxConcurrentCommands]  DEFAULT ((5)) FOR [MaxConcurrentCommands]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_ConfigUpdateIntervalSecs]  DEFAULT ((5)) FOR [ConfigUpdateIntervalSecs]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_Proxies_LogMinSeverity]  DEFAULT (N'WARNING') FOR [LogMinSeverity]
GO

ALTER TABLE [dbo].[Proxies] ADD  CONSTRAINT [DF_ChangeTime_ThreshChangeTime]  DEFAULT (getdate()) FOR [ThreshChangeTime]
GO

