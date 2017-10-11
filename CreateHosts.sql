USE [UGMon]
GO

/****** Object:  Table [dbo].[Hosts]    Script Date: 02/11/2011 10:35:00 ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[Hosts](
	[ID] [uniqueidentifier] NOT NULL,
	[LocalHostName] [nvarchar](255) NOT NULL,
	[LastSeenIP] [nvarchar](26) NOT NULL,
	[UGMonVersion] [nvarchar](12) NOT NULL,
	[VersionString] [nvarchar](max) NULL,
	[OSVersionMajor] [int] NULL,
	[OSVersionMinor] [int] NULL,
	[OSVersionBuild] [nvarchar](50) NULL,
	[ServicePack] [int] NULL,
	[Architecture] [nvarchar](32) NULL,
	[FirstSeenTime] [datetime] NOT NULL,
	[LastHelloTime] [datetime] NOT NULL,
	[ProxyHostName] [nvarchar](255) NOT NULL,
	[Platform] [nvarchar](30) NOT NULL,
	[LastKnownStatus] [int] NOT NULL,
 CONSTRAINT [PK_Hosts] PRIMARY KEY CLUSTERED 
(
	[ID] ASC
)WITH (PAD_INDEX  = OFF, STATISTICS_NORECOMPUTE  = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS  = ON, ALLOW_PAGE_LOCKS  = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

ALTER TABLE [dbo].[Hosts] ADD  CONSTRAINT [DF_Hosts_UGMonVersion]  DEFAULT (N'pre-1.11') FOR [UGMonVersion]
GO

ALTER TABLE [dbo].[Hosts] ADD  CONSTRAINT [DF_Hosts_FirstSeenTime]  DEFAULT (getdate()) FOR [FirstSeenTime]
GO

ALTER TABLE [dbo].[Hosts] ADD  CONSTRAINT [DF_Hosts_LastHelloTime]  DEFAULT (getdate()) FOR [LastHelloTime]
GO

ALTER TABLE [dbo].[Hosts] ADD  CONSTRAINT [DF_Hosts_Platform]  DEFAULT (N'UNKNOWN') FOR [Platform]
GO

ALTER TABLE [dbo].[Hosts] ADD  CONSTRAINT [DF_Hosts_LastKnownStatus]  DEFAULT ((0)) FOR [LastKnownStatus]
GO

